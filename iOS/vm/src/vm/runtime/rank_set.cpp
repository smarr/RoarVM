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


#include "headers.h"

void Rank_Set::unit_test() {
  Rank_Set rs;
  assert_always_eq(capacity(), Max_Number_Of_Cores);
  rs.verify_includes_only();

  rs.add(17);
  rs.add(23);
  rs.verify_includes_only(17, 23);

  Rank_Set rs2 = rs - 23;
  rs2.verify_includes_only(17);


  Rank_Set rs3; rs3.add(17); Rank_Set rs4 = rs - rs3;
  rs4.verify_includes_only(23);

  rs.verify_includes_only(17, 23);

  rs.remove(17);
  rs.verify_includes_only(23);
}

void Rank_Set::verify_includes_only(int a, int b) const {
  for (int r = 0;  r < capacity();  ++r)
    assert_always( r == a  ||  r == b  ?  includes(r) : !includes(r) );
}

