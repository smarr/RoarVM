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


# if On_Apple

class OSX_OS_Interface : public POSIX_OS_Interface {
public:

  static void ensure_Time_Machine_backs_up_run_directory();

# if On_iOS
  static inline void moncontrol(int) {}
# endif

  static inline void profiler_enable()  { moncontrol(1); }
  static inline void profiler_disable() { moncontrol(0); }
  static inline void profiler_clear()   {}
  
  static Power_Source get_power_source();
  
  static void pin_thread_to_core(int32_t rank);
  
};

# endif // On_Apple
