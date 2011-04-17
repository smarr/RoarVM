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


# include <gtest/gtest.h>

# include "headers.h"

/**
 * It is supposed to be filled with NULL on initialization.
 */
TEST(ExternalPrimitiveTable, Initialization) {
  External_Primitive_Table* table = new External_Primitive_Table();
  
  ASSERT_TRUE(table->size > 0);
  
  for (size_t i = 0; i < table->size; i++) {
    ASSERT_EQ(NULL, table->contents[i]);
  }
}


/**
 * Make sure that add works as expected
 */
TEST(ExternalPrimitiveTable, Add) {
  External_Primitive_Table* table = new External_Primitive_Table();
  
  ASSERT_EQ(NULL, table->contents[0]);
  
  ASSERT_EQ(1,    table->add((fn_t)4, true));
  ASSERT_EQ((fn_t)4, table->contents[0]);
  
  
  ASSERT_EQ(2,    table->add((fn_t)17, true));
  ASSERT_EQ((fn_t)17, table->contents[1]);

  ASSERT_EQ(3,    table->add((fn_t)43, true));
  ASSERT_EQ((fn_t)43, table->contents[2]);

  
  ASSERT_EQ(NULL, table->contents[3]);
}

