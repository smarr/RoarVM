/******************************************************************************
 *  Copyright (c) 2008 - 2010 IBM Corporation and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 * 
 *  Contributors:
 *    David Ungar, IBM Research - Initial Implementation
 *    Sam Adams, IBM Research - Initial Implementation
 *    Stefan Marr, Vrije Universiteit Brussel - Port to x86 Multi-Core Systems
 ******************************************************************************/


# include "headers.h"

Measurements The_Measurements;

# define  NAME_STRING(name) #name ,

const char* Measurements::labels[Measurements::N + 1] = {
  FOR_EACH_MEAS_DO(NAME_STRING)
  ""
  };
# undef NAME_STRING
int Measurements::force_op_to_complete;

u_int32 e_dummy;

Measurements::Measurements() {
  baseline = 0;

  for (int i = 0;  i < N;  ++i)
    measurements[i].init();

  GET_CYCLE_COUNT_QUICKLY(); // avoid blips
  for (int i = 0; i < 100; ++i)
    MEASURE(control, i, {});

  Measurement& cntrl = measurements[control];
  baseline = cntrl.mode_inliers(0);

  __attribute__((unused)) u_int32 dummy;
  for (int i = 0; i < 100; ++i)
    MEASURE(measurement_cycles, dummy, dummy = GET_CYCLE_COUNT_QUICKLY());
}


void Measurements::print() {
  if (!Measure)
    return;
  fprintf(stdout, "\n\nMeasurements (baseline = %ld):\n", baseline);

  print_details();
  print_summaries();
}

void Measurements::print_summaries() {
  print_config_for_spreadsheet();
  fprintf(stdout, "\n\nMeasurements summary (baseline = %ld):\n", baseline);

   fprintf(stdout, "description\tmean\tmode\tmin\t10%%\t25%%\tmedian\t75%%\t90%%\tmax\tinliers\toutliers\t< baseline\n");
   fprintf(stdout, "description\tmean\tmode\tmin\t10%%\t25%%\tmedian\t75%%\t90%%\tmax\tinliers\toutliers\t< baseline\n");


  for (int i = 0;  i < N;  ++i)
    measurements[i].print_summary(labels[i], baseline);
  fprintf(stdout, "\n");
}


void Measurements::print_details() {
  print_config_for_spreadsheet();
  fprintf(stdout, "\n\nMeasurements details (baseline = %ld):\n", baseline);

  for (int i = 0;  i < N;  ++i)
    if (measurements[i].total_inliers())
      fprintf(stdout, "\t%s", labels[i]);

  FOR_ALL_BUCKETS(b) {
    if (b < baseline) continue;
    fprintf(stdout, "\n%ld", b - baseline);
    for (int i = 0;  i < N;  ++i)
      if (measurements[i].total_inliers())
        fprintf(stdout, "\t%ld%%", measurements[i].percentage_in_bucket(b));
  }
  fprintf(stdout, "\n\n");
}


 void Measurements::Measurement::print_summary(const char* lbl, int32 baseline) {
  fprintf(stdout, "%s" "\t%.1f" "\t%ld" "\t%ld" "\t%ld" "\t%ld" "\t%ld" "\t%ld" "\t%ld" "\t%ld" "\t%lld" "\t%ld" "\t%ld",
    lbl,
    mean_inliers(baseline),
    mode_inliers(baseline),
    total_inliers() + outliers ?  min_in_or_out  :  0,
    inlier_at_percentile(baseline,  10),
    inlier_at_percentile(baseline,  25),
    inlier_at_percentile(baseline,  50),
    inlier_at_percentile(baseline,  75),
    inlier_at_percentile(baseline,  90),
    total_inliers() + outliers ?  max_in_or_out - baseline  : 0,
    total_inliers(),
    outliers,
    inliers_below_baseline(baseline));


  fprintf(stdout, "\n");
}

