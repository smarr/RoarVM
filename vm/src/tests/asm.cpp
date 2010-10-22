/******************************************************************************
 *  Copyright (c) 2008 - 2010 IBM Corporation and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 * 
 *  Contributors:
 *    Stefan Marr, Vrije Universiteit Brussel - Initial Implementation
 ******************************************************************************/


#include <gtest/gtest.h>
#include <stdint.h>

#include "asm.h"


TEST(AssemblyWrapper, XADD) {
  int32_t value = 4545;
  XADD(&value, 66);

  EXPECT_EQ(4545 + 66, value);

  value = 45;
  XADD(&value, -66);


  EXPECT_EQ(45 - 66, value);
}

TEST(AssemblyWrapper, CMPXCHG) {
  int32_t value = 456;

  int32_t result = CMPXCHG(&value, 456, 111);

  EXPECT_EQ(456, result);

  EXPECT_EQ(111, value);

  value = 333;
  result = CMPXCHG(&value, 222, 111);

  EXPECT_EQ(333, result);
  EXPECT_EQ(333, value);
}

