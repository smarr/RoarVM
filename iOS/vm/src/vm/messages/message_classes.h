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


# define DECLARE_MESSAGE_CLASS(name, superclass, constructor_formals, superconstructor_actuals, constructor_body, class_body, ack_setting, safepoint_delay_setting) \
class name##_class : public superclass##_class { \
protected: \
 acking                  get_ack_setting() const { return ack_setting; } \
 safepoint_delay_options get_safepoint_delay_setting() const { return safepoint_delay_setting; } \
 \
\
public: \
\
name##_class constructor_formals  : superclass##_class superconstructor_actuals { header = Message_Statics::name; constructor_body } \
name##_class (Receive_Marker* rm) : superclass##_class (rm) { } \
\
int size_for_transmission_and_copying() const { return sizeof(*this); } \
Message_Statics::messages get_message_type() const { return Message_Statics::name; } \
\
\
void handle_me(); \
class_body \
};


FOR_ALL_MESSAGES_DO(DECLARE_MESSAGE_CLASS)

# undef DECLARE_MESSAGE_CLASS

