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


# include <headers.h>
# include <limits.h>
# include <math.h>

FILE* BytecodeTraceFile;
extern char* displayName;
# if On_iOS
  char* displayName;
# endif

static void consume_argument(int& argc, char**& argv, int n) {
  argv[n] = argv[0];
  argv += n;
  argc -= n;
}


static bool have_set_core_count = false;

static void set_geom(char* sizes) {
#if On_Tilera
  int w, h;
  if (isdigit(w = sizes[0])  &&  sizes[1] ==','  &&  isdigit(h = sizes[2])  &&  sizes[3] == '\0' ) {
    // convert from ascii
    w = w - '0';
    h = h - '0';
    
    CPU_Coordinate::set_width_height(w,  h);
    Logical_Core::num_cores = w * h;
    have_set_core_count = true;
  }
  else
    OS_Interface::die("bad argument syntax: needs to be `-geom <digit>,<digit>'\n");
#else
  OS_Interface::die("bad argument: -geom only supported on Tilera, use -num_cores instead.'\n");
#endif
}

void set_num_cores(char* num_cores_str) {
  /* parses strings with 1 or 2 digits <digit{1,2}> */
  int num_cores;
  if (isdigit(num_cores = num_cores_str[0])  &&  num_cores_str[1] == '\0' ) {
    num_cores = num_cores - '0';
    Logical_Core::num_cores = num_cores;
    have_set_core_count = true;
    
  }
  else if (isdigit(num_cores_str[0])  &&  isdigit(num_cores_str[1])  &&  num_cores_str[2] == '\0' ) {
	  num_cores = (num_cores_str[0] - '0') * 10 + (num_cores_str[1] - '0');
	  Logical_Core::num_cores = num_cores;
    have_set_core_count = true;
  }
  else
    OS_Interface::die("bad argument syntax: needs to be `-num_cores <digit>'\n");

# if On_Tilera
  // TODO: put this into its own routine, and avoid the #if

  // Calculate the dimensions of the rectangle fitting all cores
  int w, h;
  h = sqrt(num_cores);
  w = num_cores / h;
  if (w * h < num_cores) {
    if   (w == h)  w += 1;
    else           h += 1;

    assert(w * h >= num_cores);
  }

  CPU_Coordinate::set_width_height(w,  h);
# endif
}

static void set_trace_file(char* f) {
  BytecodeTraceFile = fopen(f, "r");
  if (BytecodeTraceFile == NULL) {
    perror("bytecode trace file:");
    exit(1);
  }
}

extern int headless;
# if On_iOS
int headless = false;
# endif



# define FOR_ALL_ARGS_WITH_PARAMS_DO(template) \
template("-display",            displayName=STRING,                               "<display>") \
template("-geom",               set_geom(STRING),                                 "<digit,digit>") \
template("-num_cores",          set_num_cores(STRING),                            "<digit{1,2}>") \
template("-min_heap_MB",        Memory_System::min_heap_MB = NUMBER,              "N") \
template("-profile_after",      The_Squeak_Interpreter()->set_profile_after(NUMBER), "N") \
template("-quit_after",         The_Squeak_Interpreter()->set_quit_after(NUMBER),    "N") \
template("-round_robin_period", Memory_System::set_round_robin_period(NUMBER),    "N") \
template("-run_mask",           The_Squeak_Interpreter()->set_run_mask(NUMBER64),    "N") \
template("-trace",              set_trace_file(STRING),                           "file-name") \
template("-num_chips",          The_Squeak_Interpreter()->set_num_chips(NUMBER),    "N")



# define FOR_ALL_BOOLEAN_ARGS_DO(template) \
template("-dont_replicate_all",    Memory_System::replicate_all = false, "not replicating everything") \
template("-eschew_huge_pages",  Memory_System::use_huge_pages = false, "not using huge pages") \
template("-headless",           headless = 1, "headless") \
template("-make_checkpoint",    The_Squeak_Interpreter()->set_make_checkpoint(true), "making checkpoint") \
template("-no_fence",           The_Squeak_Interpreter()->set_fence(false), "not fencing memory on control transfers") \
template("-print_moves_to_read_write",  The_Squeak_Interpreter()->set_print_moves_to_read_write(true), "printing moves to read_write heaps") \
template("-replicate_methods",  Memory_System::replicate_methods = true, "replicating methods") \
template("-use_checkpoint",     The_Squeak_Interpreter()->set_use_checkpoint(true), "using checkpoint") \
template("-replicate_OT",       Segmented_Object_Table::replicate = true, "let hardware replicate the object table") \
template("-print_gc",           Abstract_Mark_Sweep_Collector::print_gc = true, "Print GC") \
template("-version",            print_version_info(), "Print full version information") \
template("-use_cpu_ms",         The_Squeak_Interpreter()->set_use_cpu_ms(true), "use CPU time instead of elapsed time")


static void print_version_info() {
  // compiling machine
#define stringify(s) expstr(s)
#define expstr(s) #s
  
  printf("Compiled code:\t\t%s\n", __FILE__);
#if defined(GIT_REVISION_ID)
  printf("Git Revision Id:\t%s\n", stringify(GIT_REVISION_ID));
#endif  
  printf("Compilation date:\t%s %s\n", __DATE__, __TIME__);
   
#if defined(COMPILATION_HOSTNAME)
  printf("Compiled on:\t\t%s\n", stringify(COMPILATION_HOSTNAME));
#endif
    
  printf("Used compiler:\t\t");
#if defined(__tile__)
  printf("tile-cc version %d.%d.%d\n", __TILECC__, __TILECC_MINOR__, __TILECC_PATCHLEVEL__);
#else
  printf("%s\n", __VERSION__);
#endif
  
#if defined(__OPTIMIZE__)
  printf("Optimization on\n");
#endif
  
  printf("\n\n");
  print_config();
  exit(0);
}

static void usage(char** argv) {
  fprintf(stderr, "Usage: %s", argv[0]);

# define print_arg_with_param(argName,setExpr,symbolicVal) fprintf(stderr, " [%s %s]", argName, symbolicVal);
# define print_boolean_arg(argName,setExpr,explanation) fprintf(stderr, " [%s]", argName);

  FOR_ALL_ARGS_WITH_PARAMS_DO(print_arg_with_param)
  FOR_ALL_BOOLEAN_ARGS_DO(print_boolean_arg)
# undef print_arg_with_param
# undef print_boolean_arg

    fprintf(stderr, " <snapshot-file-name.image>\n");
    exit(1);
}


static void process_arguments(int& argc, char**& argv) {
   if (argc < 2) {
    usage(argv);
  }

  bool do_print = 0 == Memory_Semantics::get_group_rank(); // it is not safe to use Logical_Core:my_rank() here, since it is not initialized yet

  for (int old_argc = -1;  old_argc != argc;  ) {
    old_argc = argc;

    # define parse_arg_with_param(argName,setExpr,symbolicVal) \
      if (strcmp(argv[1], argName) == 0) { \
        __attribute__((unused)) char* STRING = argv[2]; \
        __attribute__((unused)) int NUMBER = atoi(argv[2]); \
        __attribute__((unused)) int64 NUMBER64 = atoll(argv[2]); \
        setExpr; \
        if (do_print)  fprintf(stdout, "%s = %s\n", argName, argv[2]); \
        consume_argument(argc, argv, 2); \
       }

    # define parse_boolean_arg(argName,setExpr,explanation) \
      if (strcmp(argv[1], argName) == 0) { \
        setExpr; \
        if (do_print)  fprintf(stdout, "%s\n", explanation); \
        consume_argument(argc, argv, 1); \
      }
      FOR_ALL_ARGS_WITH_PARAMS_DO(parse_arg_with_param);
      FOR_ALL_BOOLEAN_ARGS_DO(parse_boolean_arg);

    # undef parse_arg_with_param
    # undef parse_boolean_arg
  }

  if (!have_set_core_count)
    Logical_Core::num_cores = 1;

  if (argv[1][0] == '-') {
    fprintf(stderr, "bad argument: %s\n", argv[1]);
    usage(argv);
  }
}


void helper_core_main() {
  if ( !On_Tilera )
    Memory_Semantics::initialize_local_logical_core();

  The_Memory_System()->initialize_helper();
  The_Squeak_Interpreter()->receive_initial_interpreter();
  {
    Safepoint_Ability sa(true);
    WAIT_FOR_MESSAGE(startInterpretingMessage, Logical_Core::main_rank);
  }
  extern void initip();
  initip();
  // Because BitBltPlugin only gets initialized when the literal in the ST method
  //  gets set for that prim, bit blt locally prims don't init it on all cores.
  //  Do it by brute force here. HACK -- dmu 4/09
  rvm_callInitializersInAllModules();
  Message_Statics::run_timer = true;
  The_Squeak_Interpreter()->interpret();

  char buf[BUFSIZ];
  Logical_Core::my_print_string(buf, sizeof(buf));
  lprintf( "helper finsihed: %s\n", buf);
  if ( On_Tilera ) {
    rvm_exit();
  }
}


void initialize_basic_subsystems() {
  OS_Interface::profiler_disable();
  OS_Interface::profiler_clear();
  
  OS_Interface::initialize();
  
  Memory_Semantics::initialize_timeout_timer();
  Memory_Semantics::initialize_memory_system();
  
  Printer::init_globals();
}

void read_image(char* image_path) {
  if (The_Squeak_Interpreter()->use_checkpoint())
    Squeak_Image_Reader::fake_read(image_path, The_Memory_System(), The_Squeak_Interpreter());
  else
    Squeak_Image_Reader::read(image_path, The_Memory_System(), The_Squeak_Interpreter());
}
   
void begin_interpretation() {
  if (The_Squeak_Interpreter()->make_checkpoint())
    The_Squeak_Interpreter()->save_all_to_checkpoint();
  
  assert_always(The_Squeak_Interpreter()->safepoint_ability == NULL);
  The_Squeak_Interpreter()->distribute_initial_interpreter();
  Message_Statics::run_timer = true;
  {
    Safepoint_Ability sa(true);
    The_Memory_System()->verify_if(check_assertions);
    if (check_assertions)
      The_Squeak_Interpreter()->verify_all_processes_in_scheduler();
    if (Trace_Execution) The_Squeak_Interpreter()->trace_execution();
    if (Trace_GC_For_Debugging) The_Squeak_Interpreter()->trace_for_debugging();
    startInterpretingMessage_class().send_to_other_cores();
  }
  
  if (OS_Interface::get_power_source() == OS_Interface::battery) {
    fprintf(stdout, "running on battery power: saving cycles but idle cores will slow things down\n");
    The_Squeak_Interpreter()->set_idle_cores_relinquish_cpus(true);
  }
  
  // Doesn't work yet:  The_Memory_System()->moveAllToRead_MostlyHeaps();
  The_Squeak_Interpreter()->interpret();
  ioExit();
}

void initialize_interpreter_instances_selftest_and_interpreter_proxy(char** orig_argv) {
  Memory_Semantics::go_parallel(helper_core_main, orig_argv);
  
  char buf[BUFSIZ];
  Logical_Core::my_print_string(buf, sizeof(buf));
  fprintf(stdout, "main running: %s\n", buf);
  print_time(); fprintf(stderr, "\n");
  
  
  print_config();
  
  Runtime_Tester::test();
    
  
  extern void initip();
  initip();
  
}


# if !On_iOS
int main(int argc, char *argv[]) {
  struct rlimit rl = {100000000, 100000000}; //{ RLIM_INFINITY, RLIM_INFINITY };
  // The rlimit was an attempt to get core dumps for a MDE version that broke them.
  // It didn't work, but we might need it someday. -- dmu 4/09
  if (false && setrlimit( RLIMIT_CORE, &rl)) {
    perror("setrlimit");
    fatal("rlimit");
  }
  // set_sim_tracing(SIM_TRACE_NONE);
  
  initialize_basic_subsystems();
  
  char** orig_argv = new char*[argc + 1];
  for (int i = 0;  i <= argc;  ++i)  orig_argv[i] = argv[i];
  process_arguments(argc, argv);

  if (MakeByteCodeTrace) {
    BytecodeTraceFile = fopen("bytecode_trace", "w");
    if (BytecodeTraceFile == NULL) {
      perror("bytecode trace file");
      exit(1);
    }
  }
      
  OS_Interface::ensure_Time_Machine_backs_up_run_directory();

  initialize_interpreter_instances_selftest_and_interpreter_proxy(orig_argv);
  
  char image_path[PATH_MAX];
  realpath(argv[1], image_path);
  
  read_image(image_path);
  
  extern char** environ;
  sqr_main(argc, argv, environ);
  
  begin_interpretation();
  
  return 0;
}
# endif /* ! On_iOS */


