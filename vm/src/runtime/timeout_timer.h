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


class Timeout_Timer_List_Head {
 public: // should be protected but C++ is broken
  Timeout_Timer_List_Head* next;
  Timeout_Timer_List_Head* prev;
  const char* why;

  void remove_me()  { prev->next = next;  next->prev = prev;}
  void add_me(Timeout_Timer_List_Head* x)     { next = x->next; prev = x;  next->prev = prev->next = this; }
  void init_links() { next = prev = this; }

  Timeout_Timer_List_Head()                   { init_links(); why = "head"; }
};


class Timeout_Timer : public Timeout_Timer_List_Head {
 protected:
  // STEFAN: TODO: Read this from sysctl on osx or /proc/cpuinfo on linux
  /*
   long long get_cycles_persecond() {
   FILE *f;
   double result;
   int s;

   f = fopen("/proc/cpuinfo","r");
   if (!f) return 0;

   for (;;) {
   s = fscanf(f,"cpu MHz : %lf",&result);
   if (s > 0) break;
   if (s == 0) s = fscanf(f,"%*[^\n]\n");
   if (s < 0) { result = 0; break; }
   }

   fclose(f);
   return 1000000.0 * result;
   }
   */
  static const u_int64 cycles_per_sec =
# if On_Tilera
   660000000LL;
# else
  2800000000LL;
# endif


  // introduced for thread-based version, as abstraction for threadlocal
  // timeout lists
  static Timeout_Timer_List_Head* get_head();

# if On_Tilera  || Force_Direct_Timeout_Timer_List_Head_Access
private:
  static Timeout_Timer_List_Head _head;
public:
  static void init_threadlocal() {}
  
# else
private:
  static void _dtor_threadlocal(void* local_head);
  static pthread_key_t threadlocal_head;
public:
  static void init_threadlocal();
# endif

  u_int64 start_time;
  int timeout_secs;
  u_int64 timeout_cycles;
  static const u_int64 never = ~0; // cannot be 0, that's what get_cycle_count returns when stubbed out -- dmu 5/10
  static const u_int64 default_timeout_secs = 30;
  static const int any = -1;

  int who_I_am_waiting_for;

  virtual void complain();
  virtual void act();


public:
  static void initialize();

  Timeout_Timer(const char* w)                { add_me(get_head()); why = w; stop(); timeout_secs = default_timeout_secs;  timeout_cycles = timeout_secs * cycles_per_sec; who_I_am_waiting_for = any; }
  Timeout_Timer(const char* w, int ts)        { add_me(get_head()); why = w; stop(); timeout_secs = ts;  timeout_cycles = timeout_secs * cycles_per_sec;  who_I_am_waiting_for = any; }
  Timeout_Timer(const char* w, int ts, int r) { add_me(get_head()); why = w; stop(); timeout_secs = ts;  timeout_cycles = timeout_secs * cycles_per_sec;  who_I_am_waiting_for = r; }
  ~Timeout_Timer() { stop(); remove_me(); }

  void start() { start_time = OS_Interface::get_cycle_count(); assert(is_running()); }
  void stop()  { start_time = never; assert(!is_running()); }
  void check();
  void restart();
  bool is_running() { return start_time != never; }
  bool has_timed_out() { return is_running()  &&  elapsed_cycles()  >  timeout_cycles; }
  u_int64 elapsed_cycles()  { return OS_Interface::get_cycle_count() - start_time; }
  u_int64 elapsed_seconds() { return elapsed_cycles() / cycles_per_sec; }
  static void check_all();
  static void restart_all();

# define FOR_ALL_TIMEOUT_TIMERS(p) \
  Timeout_Timer_List_Head* head = get_head(); \
  for ( Timeout_Timer* p  = (Timeout_Timer*)head->next;  \
                       p != (Timeout_Timer*)head; \
                       p  = (Timeout_Timer*)p->next)

  bool is_on_list() {
    FOR_ALL_TIMEOUT_TIMERS(tt)
      if (tt == this) return true;
    return false;
  }
};


class Safepoint_Acquisition_Timer: public Timeout_Timer {
  protected:
  virtual void complain();
  virtual void act();
 public:
  Safepoint_Acquisition_Timer() : Timeout_Timer("safepoint acquisition", 60) {}
};

