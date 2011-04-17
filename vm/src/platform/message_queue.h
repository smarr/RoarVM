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


# if   On_Tilera

class   ILib_Message_Queue;
typedef ILib_Message_Queue                     Message_Queue;

# elif Use_PerSender_Message_Queue

class   Shared_Memory_Message_Queue_Per_Sender;
typedef Shared_Memory_Message_Queue_Per_Sender Message_Queue;

# else

class   Shared_Memory_Message_Queue;
typedef Shared_Memory_Message_Queue            Message_Queue;

# endif
