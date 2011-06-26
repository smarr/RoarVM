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


# if Multiple_Tileras

class Host_PCI_Info {


#   define FOR_ALL_INFOS(template) \
      template(Host_Link_Index) \
      template(Max_Payload_Size) \
      template(Max_Read_Size) \
      template(Link_Width) \
      template(LINK_BAR1_SIZE) \
      template(LINK_BAR1_ADDRESS) \
      template(PREBOOTER_VERSION)
  
  void read_info_file();
  
  public:

#   define declare_info_var(name) int name;
        FOR_ALL_INFOS(declare_info_var)
#   undef declare_info_var
  
  Host_PCI_Info();
  void print();

};


# endif
