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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "liblte/phy/phy.h"

#ifndef DISABLE_UHD
#include "liblte/cuhd/cuhd.h"
void *uhd;
#endif

char *output_file_name = NULL;

lte_cell_t cell = {
  6,            // nof_prb
  1,            // nof_ports
  1,            // cell_id
  CPNORM        // cyclic prefix
};
  
uint32_t cfi=1;
uint32_t mcs_idx = 12;
int nof_frames = -1;

char *uhd_args = "";
float uhd_amp = 0.25, uhd_gain = 10.0, uhd_freq = 2400000000;

filesink_t fsink;
lte_fft_t ifft;
pbch_t pbch;
pcfich_t pcfich;
pdcch_t pdcch;
pdsch_t pdsch;
pdsch_harq_t harq_process;
regs_t regs;

cf_t *sf_buffer = NULL, *output_buffer = NULL;
int sf_n_re, sf_n_samples;

void usage(char *prog) {
  printf("Usage: %s [agmfoncvp]\n", prog);
#ifndef DISABLE_UHD
  printf("\t-a UHD args [Default %s]\n", uhd_args);
  printf("\t-g UHD TX gain [Default %.2f dB]\n", uhd_gain);
  printf("\t-f UHD TX frequency [Default %.1f MHz]\n", uhd_freq / 1000000);
#else
  printf("\t   UHD is disabled. CUHD library not available\n");
#endif
  printf("\t-o output_file [Default USRP]\n");
  printf("\t-m MCS index [Default %d]\n", mcs_idx);
  printf("\t-n number of frames [Default %d]\n", nof_frames);
  printf("\t-c cell id [Default %d]\n", cell.id);
  printf("\t-p nof_prb [Default %d]\n", cell.nof_prb);
  printf("\t-v [set verbose to debug, default none]\n");
}

void parse_args(int argc, char **argv) {
  int opt;
  while ((opt = getopt(argc, argv, "agfmoncpv")) != -1) {
    switch (opt) {
    case 'a':
      uhd_args = argv[optind];
      break;
    case 'g':
      uhd_gain = atof(argv[optind]);
      break;
    case 'f':
      uhd_freq = atof(argv[optind]);
      break;
    case 'o':
      output_file_name = argv[optind];
      break;
    case 'm':
      mcs_idx = atoi(argv[optind]);
      break;
    case 'n':
      nof_frames = atoi(argv[optind]);
      break;
    case 'p':
      cell.nof_prb = atoi(argv[optind]);
      break;
    case 'c':
      cell.id = atoi(argv[optind]);
      break;
    case 'v':
      verbose++;
      break;
    default:
      usage(argv[0]);
      exit(-1);
    }
  }
#ifdef DISABLE_UHD
  if (!output_file_name) {
    usage(argv[0]);
    exit(-1);
  }
#endif
}

void base_init() {
  
  /* init memory */
  sf_buffer = malloc(sizeof(cf_t) * sf_n_re);
  if (!sf_buffer) {
    perror("malloc");
    exit(-1);
  }
  output_buffer = malloc(sizeof(cf_t) * sf_n_samples);
  if (!output_buffer) {
    perror("malloc");
    exit(-1);
  }
  /* open file or USRP */
  if (output_file_name) {
    if (filesink_init(&fsink, output_file_name, COMPLEX_FLOAT_BIN)) {
      fprintf(stderr, "Error opening file %s\n", output_file_name);
      exit(-1);
    }
  } else {
#ifndef DISABLE_UHD
    printf("Opening UHD device...\n");
    if (cuhd_open(uhd_args, &uhd)) {
      fprintf(stderr, "Error opening uhd\n");
      exit(-1);
    }
#else
    printf("Error UHD not available. Select an output file\n");
    exit(-1);
#endif
  }

  /* create ifft object */
  if (lte_ifft_init(&ifft, CPNORM, cell.nof_prb)) {
    fprintf(stderr, "Error creating iFFT object\n");
    exit(-1);
  }
  if (pbch_init(&pbch, cell)) {
    fprintf(stderr, "Error creating PBCH object\n");
    exit(-1);
  }

  if (regs_init(&regs, R_1, PHICH_NORM, cell)) {
    fprintf(stderr, "Error initiating regs\n");
    exit(-1);
  }

  if (pcfich_init(&pcfich, &regs, cell)) {
    fprintf(stderr, "Error creating PBCH object\n");
    exit(-1);
  }

  if (regs_set_cfi(&regs, cfi)) {
    fprintf(stderr, "Error setting CFI\n");
    exit(-1);
  }

  if (pdcch_init(&pdcch, &regs, cell)) {
    fprintf(stderr, "Error creating PDCCH object\n");
    exit(-1);
  }

  if (pdsch_init(&pdsch, cell)) {
    fprintf(stderr, "Error creating PDSCH object\n");
    exit(-1);
  }
  
  pdsch_set_rnti(&pdsch, 1234);
  
  if (pdsch_harq_init(&harq_process, &pdsch)) {
    fprintf(stderr, "Error initiating HARQ process\n");
    exit(-1);
  }
}

void base_free() {

  pdsch_harq_free(&harq_process);
  pdsch_free(&pdsch);
  pdcch_free(&pdcch);
  regs_free(&regs);
  pbch_free(&pbch);

  lte_ifft_free(&ifft);

  if (sf_buffer) {
    free(sf_buffer);
  }
  if (output_buffer) {
    free(output_buffer);
  }
  if (output_file_name) {
    filesink_free(&fsink);
  } else {
#ifndef DISABLE_UHD
    cuhd_close(&uhd);
#endif
  }
}

int main(int argc, char **argv) {
  int nf, sf_idx, N_id_2;
  cf_t pss_signal[PSS_LEN];
  float sss_signal0[SSS_LEN]; // for subframe 0
  float sss_signal5[SSS_LEN]; // for subframe 5
  pbch_mib_t mib;
  ra_pdsch_t ra_dl;
  ra_prb_t prb_alloc;
  refsignal_t refs[NSLOTS_X_FRAME];
  int i, n;
  char *data;
  cf_t *sf_symbols[MAX_PORTS];
  dci_msg_t dci_msg;
  dci_location_t locations[NSUBFRAMES_X_FRAME][10];

#ifdef DISABLE_UHD
  if (argc < 3) {
    usage(argv[0]);
    exit(-1);
  }
#endif

  parse_args(argc, argv);

  N_id_2 = cell.id % 3;
  sf_n_re = 2 * CPNORM_NSYMB * cell.nof_prb * RE_X_RB;
  sf_n_samples = 2 * SLOT_LEN_CPNORM(lte_symbol_sz(cell.nof_prb));

  /* this *must* be called after setting slot_len_* */
  base_init();

  /* Generate PSS/SSS signals */
  pss_generate(pss_signal, N_id_2);
  sss_generate(sss_signal0, sss_signal5, cell.id);
  
  /* Generate CRS signals */
  for (i = 0; i < NSLOTS_X_FRAME; i++) {
    if (refsignal_init_LTEDL(&refs[i], 0, i, cell)) {
      fprintf(stderr, "Error initiating CRS slot=%d\n", i);
      return -1;
    }
  }

  mib.nof_ports = cell.nof_ports;
  mib.nof_prb = cell.nof_prb;
  mib.phich_length = PHICH_NORM;
  mib.phich_resources = R_1;
  mib.sfn = 0;

  for (i = 0; i < MAX_PORTS; i++) { // now there's only 1 port
    sf_symbols[i] = sf_buffer;
  }

#ifndef DISABLE_UHD
  if (!output_file_name) {
    printf("Set TX rate: %.2f MHz\n",
        cuhd_set_tx_srate(uhd, lte_sampling_freq_hz(cell.nof_prb)) / 1000000);
    printf("Set TX gain: %.1f dB\n", cuhd_set_tx_gain(uhd, uhd_gain));
    printf("Set TX freq: %.2f MHz\n",
        cuhd_set_tx_freq(uhd, uhd_freq) / 1000000);
  }
#endif

  bzero(&ra_dl, sizeof(ra_pdsch_t));
  ra_dl.harq_process = 0;
  ra_dl.mcs_idx = mcs_idx;
  ra_dl.ndi = 0;
  ra_dl.rv_idx = 0;
  ra_dl.alloc_type = alloc_type0;
  ra_dl.type0_alloc.rbg_bitmask = 0xffffffff;
  
  dci_msg_pack_pdsch(&ra_dl, &dci_msg, Format1, cell.nof_prb, false);
  
  ra_prb_get_dl(&prb_alloc, &ra_dl, cell.nof_prb);
  ra_prb_get_re_dl(&prb_alloc, cell.nof_prb, 1, cell.nof_prb<10?(cfi+1):cfi, CPNORM);
  ra_mcs_from_idx_dl(mcs_idx, cell.nof_prb, &ra_dl.mcs);

  ra_pdsch_fprint(stdout, &ra_dl, cell.nof_prb);
  
  /* Initiate valid DCI locations */
  for (i=0;i<NSUBFRAMES_X_FRAME;i++) {
    pdcch_ue_locations(&pdcch, locations[i], 10, i, cfi, 1234);
  }

  data = malloc(sizeof(char) * ra_dl.mcs.tbs);
  if (!data) {
    perror("malloc");
    exit(-1);
  }  
    
  nf = 0;
  
  if (pdsch_harq_setup(&harq_process, ra_dl.mcs, &prb_alloc)) {
    fprintf(stderr, "Error configuring HARQ process\n");
    exit(-1);
  }

  while (nf < nof_frames || nof_frames == -1) {
    for (sf_idx = 0; sf_idx < NSUBFRAMES_X_FRAME && (nf < nof_frames || nof_frames == -1); sf_idx++) {
      bzero(sf_buffer, sizeof(cf_t) * sf_n_re);

      if (sf_idx == 0 || sf_idx == 5) {
        pss_put_slot(pss_signal, sf_buffer, cell.nof_prb, CPNORM);
        sss_put_slot(sf_idx ? sss_signal5 : sss_signal0, sf_buffer, cell.nof_prb,
            CPNORM);
      }
      
      if (sf_idx == 0) {
        pbch_encode(&pbch, &mib, sf_symbols);
      }
    
      for (n=0;n<2;n++) {
        refsignal_put(&refs[2*sf_idx+n], &sf_buffer[n*sf_n_re/2]);
      }

      pcfich_encode(&pcfich, cfi, sf_symbols, sf_idx);       

      INFO("SF: %d, Generating %d random bits\n", sf_idx, ra_dl.mcs.tbs);
      for (i=0;i<ra_dl.mcs.tbs;i++) {
        data[i] = rand()%2;
      }
      
      INFO("Puttting DCI to location: n=%d, L=%d\n", locations[sf_idx][0].ncce, locations[sf_idx][0].L);
      if (pdcch_encode(&pdcch, &dci_msg, locations[sf_idx][0], 1234, sf_symbols, sf_idx, cfi)) {
        fprintf(stderr, "Error encoding DCI message\n");
        exit(-1);
      }
      
      pdsch_encode(&pdsch, data, sf_symbols, sf_idx, &harq_process, ra_dl.rv_idx);        

      /* Transform to OFDM symbols */
      lte_ifft_run_sf(&ifft, sf_buffer, output_buffer);
      
      /* send to file or usrp */
      if (output_file_name) {
        filesink_write(&fsink, output_buffer, sf_n_samples);
        usleep(5000);
      } else {
#ifndef DISABLE_UHD
        vec_sc_prod_cfc(output_buffer, uhd_amp, output_buffer, sf_n_samples);
        cuhd_send(uhd, output_buffer, sf_n_samples, true);
#endif
      }
      nf++;
    }
    mib.sfn = (mib.sfn + 1) % 1024;
    printf("SFN: %4d\r", mib.sfn);
    fflush(stdout);
  }

  base_free();

  printf("Done\n");
  exit(0);
}


