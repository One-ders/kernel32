

#ifdef SECTION_BEGIN
//void *init_func_begin __attribute__((section("init_funcs")))=0;
asm(".global init_func_begin");
asm(".section .init_funcs,\"a\",%progbits\n\t"
    "init_func_begin:\n\t");
#endif

#ifdef SECTION_END
//void *init_func_end __attribute__((section("init_funcs")))=0;
asm(".global init_func_end");
asm(".section .init_funcs,\"a\",%progbits\n\t"
    "init_func_end:\n\t" );
#endif
