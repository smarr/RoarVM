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


extern Logical_Core* logical_cores;
extern Logical_Core gc_core;

class Logical_Core {
private:
  int     _rank;
  u_int64 _rank_mask;
  
public:
  Message_Queue  message_queue;
  CPU_Coordinate coordinate;
  
  void initialize(int rank) {
    _rank = rank;
    _rank_mask = 1LL << u_int64(rank);
    coordinate.initialize(rank);
  }
  
  inline int      rank()      const { assert(this != NULL); return _rank; }
  inline u_int64  rank_mask() const { assert(this != NULL); return _rank_mask; }
  inline bool     is_main()   const { return main_rank == _rank; }
  
  void print_string(char* buf, int buf_size) {
    char coord[16] = { 0 };
    
    if (coordinate.print(coord, 16))
      snprintf(buf, buf_size,
             "%s: [%s], %d of %d (%d remaining)\n",
             (is_main() ? "main" : "helper"),
             coord, _rank,
             group_size, remaining);
    else
      snprintf(buf, buf_size,
               "%s: %d of %d (%d remaining)\n",
               (is_main() ? "main" : "helper"),
               _rank,
               group_size, remaining);
  }
  

// static:
  
  static int num_cores;     // threadsafe, read only after init, Stefan: 2009-09-06
  static int main_rank;     // threadsafe, read only after init, Stefan: 2009-09-06
  static int group_size;    // group_size is different from num_cores, since it indicates the actually
                            // used number of logical cores, instead of the desired num_cores
                            // Should be only used to guide initialization  
  static int remaining;     // Indicates the remaining number of physical cores, those not executing a logical_core/interpreter

  static int group_size_GC() { return group_size + 1; }  // The actually used number of logical cores, including the GC core.

  
  static inline bool is_initialized()   { return Memory_Semantics::cores_are_initialized(); }
  
  static inline Logical_Core* my_core() { return Memory_Semantics::my_core();      }
  static inline int           my_rank() { return Memory_Semantics::my_rank();      }
  static inline u_int64  my_rank_mask() { return Memory_Semantics::my_rank_mask(); }
  
  static void my_print_string(char* buf, int buf_size) {
    my_core()->print_string(buf, buf_size);
  }
  
  
  static void initialize_all_cores();
    
  static void initialize_GC_core();
  static void start_GC_thread();
  static Logical_Core* get_GC_core(){ return &gc_core; }
  
  static inline Logical_Core* main_core() { return &logical_cores[main_rank]; }
  static inline bool running_on_main() {
    return main_rank == my_rank();
  }

    static inline bool running_on_GC() {
        return group_size == my_rank();
    }
    
    static inline bool is_rank_of_GC(int rank){
        return group_size == rank;
    }
};

# define FOR_ALL_RANKS(r) \
  for (int r = 0;  r < Logical_Core::group_size;  ++r)

# define FOR_ALL_OTHER_RANKS(r) \
  FOR_ALL_RANKS(r) if (r != Logical_Core::my_rank())

# define FOR_ALL_RANKS_AND_GC(r) \
  for (int r = Logical_Core::group_size;  r >= 0;  --r)

# define FOR_ALL_OTHER_RANKS_AND_GC(r) \
  FOR_ALL_RANKS_AND_GC(r) if (r != Logical_Core::my_rank())

# define FOR_ALL_RANKS_IN_REVERSE_ORDER(r) \
  for (int r = Logical_Core::group_size - 1;  r >= 0;  --r)

