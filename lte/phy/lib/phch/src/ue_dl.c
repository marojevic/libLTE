/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2014 The libLTE Developers. See the
 * COPYRIGHT file at the top-level directory of this distribution.
 *
 * \section LICENSE
 *
 * This file is part of the libLTE library.
 *
 * libLTE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * libLTE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include "liblte/phy/phch/ue_dl.h"


#define EXPAVERAGE(data, average, nframes) ((data + average * nframes) / (nframes + 1))  

#define CURRENT_FFTSIZE   lte_symbol_sz(q->cell.nof_prb)
#define CURRENT_SFLEN     SF_LEN(CURRENT_FFTSIZE, q->cell.cp)

#define CURRENT_SLOTLEN_RE SLOT_LEN_RE(q->cell.nof_prb, q->cell.cp)
#define CURRENT_SFLEN_RE SF_LEN_RE(q->cell.nof_prb, q->cell.cp)


int ue_dl_init(ue_dl_t *q, 
               lte_cell_t cell,  
               phich_resources_t phich_resources, phich_length_t phich_length, 
               uint16_t user_rnti) 
{
  int ret = LIBLTE_ERROR_INVALID_INPUTS; 
  
  if (q                 != NULL &&
      lte_cell_isvalid(&cell))   
  {
    ret = LIBLTE_ERROR;
    
    q->cell = cell; 
    q->user_rnti = user_rnti; 
    q->pkt_errors = 0;
    q->pkts_total = 0; 
    q->nof_trials = 0;
    
    if (lte_fft_init(&q->fft, q->cell.cp, q->cell.nof_prb)) {
      fprintf(stderr, "Error initiating FFT\n");
      goto clean_exit;
    }
    if (chest_init_LTEDL(&q->chest, cell)) {
      fprintf(stderr, "Error initiating channel estimator\n");
      goto clean_exit;
    }
    if (regs_init(&q->regs, phich_resources, phich_length, q->cell)) {
      fprintf(stderr, "Error initiating REGs\n");
      goto clean_exit;
    }

    if (pcfich_init(&q->pcfich, &q->regs, q->cell)) {
      fprintf(stderr, "Error creating PCFICH object\n");
      goto clean_exit;
    }

    if (pdcch_init(&q->pdcch, &q->regs, q->cell)) {
      fprintf(stderr, "Error creating PDCCH object\n");
      goto clean_exit;
    }

    if (pdsch_init(&q->pdsch, q->cell)) {
      fprintf(stderr, "Error creating PDSCH object\n");
      goto clean_exit;
    }
    for (uint32_t i=0;i<NOF_HARQ_PROCESSES; i++) {
      if (pdsch_harq_init(&q->harq_process[i], &q->pdsch)) {
        fprintf(stderr, "Error initiating HARQ process\n");
        goto clean_exit;
      }
    }
    q->sf_symbols = vec_malloc(CURRENT_SFLEN_RE * sizeof(cf_t));
    if (!q->sf_symbols) {
      perror("malloc");
      goto clean_exit; 
    }
    for (uint32_t i=0;i<q->cell.nof_ports;i++) {
      q->ce[i] = vec_malloc(CURRENT_SFLEN_RE * sizeof(cf_t));
      if (!q->ce[i]) {
        perror("malloc");
        goto clean_exit; 
      }
    }
    
    ret = LIBLTE_SUCCESS;
  } else {
    fprintf(stderr, "Invalid cell properties: Id=%d, Ports=%d, PRBs=%d\n",
            cell.id, cell.nof_ports, cell.nof_prb);      
  }

clean_exit: 
  if (ret == LIBLTE_ERROR) {
    ue_dl_free(q);
  }
  return ret;
}

void ue_dl_free(ue_dl_t *q) {
  if (q) {
    lte_fft_free(&q->fft);
    chest_free(&q->chest);
    regs_free(&q->regs);
    pcfich_free(&q->pcfich);
    pdcch_free(&q->pdcch);
    pdsch_free(&q->pdsch);
    for (uint32_t i=0;i<NOF_HARQ_PROCESSES; i++) {
      pdsch_harq_free(&q->harq_process[i]);
    }
    if (q->sf_symbols) {
      free(q->sf_symbols);
    }
    for (uint32_t i=0;i<q->cell.nof_ports;i++) {
      if (q->ce[i]) {
        free(q->ce[i]);
      }
    }
  }
}

int ue_dl_receive(ue_dl_t *q, cf_t *input, char *data, uint32_t sf_idx, uint32_t sfn, uint16_t rnti) 
{
  uint32_t cfi, cfi_distance, i;
  ra_pdsch_t ra_dl;
  dci_location_t locations[10];
  dci_msg_t dci_msg;
  uint32_t nof_locations;
  uint16_t crc_rem; 
  dci_format_t format; 
  
  /* If we are looking for SI Blocks, search only in appropiate places */
  if ((rnti == SIRNTI && (sfn % 2) == 0 && sf_idx == 5) ||
       rnti != SIRNTI)
  {
    
    /* Run FFT for all subframe data */
    lte_fft_run_sf(&q->fft, input, q->sf_symbols);

    /* Get channel estimates for each port */
    chest_ce_sf(&q->chest, q->sf_symbols, q->ce, sf_idx);
    
    /* First decode PCFICH and obtain CFI */
    if (pcfich_decode(&q->pcfich, q->sf_symbols, q->ce, sf_idx, &cfi, &cfi_distance)<0) {
      fprintf(stderr, "Error decoding PCFICH\n");
      return LIBLTE_ERROR;
    }

    INFO("Decoded CFI=%d with distance %d\n", cfi, cfi_distance);

    if (regs_set_cfi(&q->regs, cfi)) {
      fprintf(stderr, "Error setting CFI\n");
      return LIBLTE_ERROR;
    }
    
    /* Generate PDCCH candidates */
    if (rnti == SIRNTI) {
      nof_locations = pdcch_common_locations(&q->pdcch, locations, 10, cfi);
      format = Format1A; 
    } else {
      nof_locations = pdcch_ue_locations(&q->pdcch, locations, 10, sf_idx, cfi, q->user_rnti);    
      format = Format1;
    }

    crc_rem = 0;
    for (i=0;i<nof_locations && crc_rem != rnti;i++) {
      if (pdcch_extract_llr(&q->pdcch, q->sf_symbols, q->ce, locations[i], sf_idx, cfi)) {
        fprintf(stderr, "Error extracting LLRs\n");
        return LIBLTE_ERROR;
      }
      if (pdcch_decode_msg(&q->pdcch, &dci_msg, format, &crc_rem)) {
        fprintf(stderr, "Error decoding DCI msg\n");
        return LIBLTE_ERROR;
      }
      INFO("Decoded DCI message RNTI: 0x%x\n", crc_rem);
    }

    
    if (crc_rem == rnti) {
      if (dci_msg_to_ra_dl(&dci_msg, rnti, q->user_rnti, q->cell, cfi, &ra_dl)) {
        fprintf(stderr, "Error unpacking PDSCH scheduling DCI message\n");
        return LIBLTE_ERROR;
      }

      uint32_t rvidx; 
      if (rnti == SIRNTI) {
        switch((sfn%8)/2) {
          case 0: 
            rvidx = 0; 
            break;
          case 1:
            rvidx = 2;
            break;
          case 2:
            rvidx = 3;
            break;
          case 3:
            rvidx = 1; 
            break;
        }
      } else {
        rvidx = ra_dl.rv_idx;
      }
      
      if (rvidx == 0) {
        if (pdsch_harq_setup(&q->harq_process[0], ra_dl.mcs, &ra_dl.prb_alloc)) {
          fprintf(stderr, "Error configuring HARQ process\n");
          return LIBLTE_ERROR;
        }
      }
      if (q->harq_process[0].mcs.mod > 0) {
        if (pdsch_decode(&q->pdsch, q->sf_symbols, q->ce, data, sf_idx, 
            &q->harq_process[0], rvidx)) {
          if (rnti == SIRNTI && rvidx == 1) {
            q->pkt_errors++;
          } else {
            q->pkt_errors++;                
          }            
        } else {
          if (VERBOSE_ISINFO()) {
            INFO("Decoded Message: ", 0);
            vec_fprint_hex(stdout, data, ra_dl.mcs.tbs);
          }
        }
        if (rnti == SIRNTI && rvidx == 1) {
          q->pkts_total++;                      
        }
      }
    }
    if (rnti == SIRNTI && (sfn%8) == 0) {
      q->nof_trials++;      
    }
  }
  
  if (crc_rem == rnti) {        
    return ra_dl.mcs.tbs;    
  } else {
    return 0;
  }
}
