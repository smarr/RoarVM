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


#ifndef MEASUREMENTS_H_
#define MEASUREMENTS_H_




class Measurements {
  public:

  static int force_op_to_complete;


  public:

  # define FOR_EACH_MEAS_DO(template) \
    template(control) \
    template(measurement_cycles) \
    template(object_table_accesses) \
    template(fetch_pointer) \
    template(send_point_to_point_message) \
    template(receive_point_to_point_message) \
    template(send_bufchan_message) \
    template(receive_bufchan_message) \
    template(release_bufchan_message) \
    template(send_strchan) \
    template(receive_strchan) \
    template(send_rawchan) \
    template(receive_rawchan) \
    template(context_initialization)

  # define ENUM_ELEM(name) name,
  enum what {
    FOR_EACH_MEAS_DO(ENUM_ELEM)
    N };
  # undef ENUM_ELEM


  static const char* labels[N+1];
  class Measurement {
     public:

    static const int largest_bucket = 500;
    # define FOR_ALL_BUCKETS(i) for (int i = 0;  i <= Measurements::Measurement::largest_bucket;  ++i)

    int32 cycles[largest_bucket + 1], outliers, min_in_or_out, max_in_or_out;
    void init() {
      for (int i = 0;  i <= largest_bucket;  ++i)
        cycles[i] = 0;
      outliers = 0;
      max_in_or_out = 0x80000000;
      min_in_or_out = max_in_or_out - 1;
    }

    void update(OS_Interface::get_cycle_count_quickly_t c1, OS_Interface::get_cycle_count_quickly_t c2) {
      int32 c = int32(c2 - c1);
      if (c > largest_bucket  ||  c < 0)
        ++outliers;
      else
        ++cycles[c];
      if (min_in_or_out > c) min_in_or_out = c;
      if (max_in_or_out < c) max_in_or_out = c;
    }


    int64 total_inliers() {
      int32 s = 0;
      FOR_ALL_BUCKETS(i) s += cycles[i];
      return s;
    }
    int32 inlier_at_percentile(int32 baseline, int32 percentile) {
      int64 part_sum = 0, target = max(1, total_inliers() * (int64)percentile / (int64)100);
      FOR_ALL_BUCKETS(i) {
        part_sum += cycles[i];
        if (part_sum >= target)
          return i - baseline;
      }
     return 0;
    }
    double mean_inliers(int32 baseline) {
      int64 w_sum = 0;
      FOR_ALL_BUCKETS(i) w_sum += i * cycles[i];
      int64 t = total_inliers();
      if (t == 0LL) return 0.0;
      return double(w_sum) / t  -  baseline;
    }
    int32 mode_inliers(int32 baseline) {
      int32 m = 0;
      FOR_ALL_BUCKETS(i)
        if (cycles[i] > cycles[m]) m = i;
      if (cycles[m] == 0)  return 0;
      return m - baseline;
    }
    int32 inliers_below_baseline(int32 baseline) {
      int32 s = 0;
      FOR_ALL_BUCKETS(i) {
        if (i >= baseline) break;
        s += cycles[i];
      }
      return s;
    }

    int32 percentage_in_bucket(int32 bucket) {
      return cycles[bucket] * 100 / max(1, total_inliers());
    }



    void print_summary(const char* lbl, int32 baseline);
   } measurements[N];
  int32 baseline; // overhead of basic measurement

  Measurements();


  void print(), print_summaries(), print_details();
};

extern Measurements The_Measurements;



# if Measure

// take a value argument, to force the load to complete

# define MEASURE(what, value, stmtOrBlock) \
  if (0) ; else { \
    OS_Interface::get_cycle_count_quickly_t c1 = GET_CYCLE_COUNT_QUICKLY(); \
    stmtOrBlock; \
    Measurements::force_op_to_complete = (int)(value); \
    OS_Interface::get_cycle_count_quickly_t c2 = GET_CYCLE_COUNT_QUICKLY(); \
    The_Measurements.measurements[Measurements::what].update(c1, c2); \
  }


# else

# define MEASURE(what, value, stmtOrBlock) \
  if (0) ; else { stmtOrBlock; }

# endif

#endif /*MEASUREMENTS_H_*/

