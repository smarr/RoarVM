/******************************************************************************
 *  Copyright (c) 2008 - 2011 IBM Corporation and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 * 
 *  Contributors:
 *    Mattias De Wael, Vrije Universiteit Brussel - Parallel Garbage Collection
 *    Wouter Amerijckx, Vrije Universiteit Brussel - Parallel Garbage Collection
 ******************************************************************************/


class Checkpoint_Closure {
public:
    Checkpoint_Closure() {}
    virtual void doIt() = 0;
    Checkpoint_Closure* responseClosure;
    virtual const char* class_name(char*) { return "Checkpoint_Closure"; }
    
};

class Empty_Checkpoint_Closure: public Checkpoint_Closure {
public:
    void doIt() {
        printf("Printing from inside closure\n");
        responseClosure = new Empty_Checkpoint_Closure();
        
    }
    virtual const char* class_name(char*) { return "Empty_Checkpoint_Closure"; }
};

class Print_Closure: public Checkpoint_Closure {
public:
    void doIt() {
        printf("Printing from inside closure\n");
        responseClosure = new Empty_Checkpoint_Closure();
    }
    virtual const char* class_name(char*) { return "Print_Closure"; }
};

