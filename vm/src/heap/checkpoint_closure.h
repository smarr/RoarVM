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

