bool on_NMT_trap(Oop* p, Oop value);

void on_Protected_trap(Oop* p, Oop oldValue);

bool is_pointing_to_protected_page_slowVersion(Oop oop);

bool is_pointing_to_protected_page(Oop oop);

void doLVB(Oop* p);
