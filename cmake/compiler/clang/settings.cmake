#todo: minimal version?

# Set build-directive (used in core to tell which buildtype we used)
target_compile_definitions(trinity-compile-option-interface
   INTERFACE
     -D_BUILD_DIRECTIVE="${CMAKE_BUILD_TYPE}")

if(DO_WARN)
  target_compile_options(trinity-warning-interface
    INTERFACE
        -W
        -Wall
        -Wextra
        -Winit-self
        -Wfatal-errors
        -Wno-mismatched-tags
        -Woverloaded-virtual)
 
  message(STATUS "Clang: All warnings enabled")
endif()

if(DO_DEBUG)
  target_compile_options(trinity-compile-option-interface
    INTERFACE
        -g3)
	  
  message(STATUS "Clang: Debug-flags set (-g3)")

  #http://clang.llvm.org/docs/AddressSanitizer.html
  if(CLANG_ADDRESS_SANITIZER)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize=address -fno-sanitize-recover=undefined,integer -fsanitize=return -fsanitize=bounds -fsanitize=shift -fsanitize=bool -fsanitize=enum")	
    message(STATUS "/!\\ Clang: AddressSanitizer enabled. Except SLOWDOWNS on runtime.")
  endif() 
  if(CLANG_THREAD_SANITIZER)
    #Q: When I run the program under gdb, it says: FATAL: ThreadSanitizer can not mmap the shadow memory (something is mapped at 0x555555554000 < 0x7cf000000000). What to do? Run as:
    # gdb -ex 'set disable-randomization off'

    if(USE_GPERFTOOLS)
        MESSAGE(SEND_ERROR "CLANG_THREAD_SANITIZER cannot be enabled with gperftools, it would conflict with tcmalloc")
    endif()     
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fPIE")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")
    message(STATUS "/!\\ Clang: ThreadSanitizer enabled. Expect LARGE SLOWDOWNS on runtime.")
  endif()
  if(CLANG_MEMORY_SANITIZER)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory -fno-omit-frame-pointer")	
    message(STATUS "/!\\ Clang: MemorySanitizer enabled. Except SLOWDOWNS on runtime.")
  endif()
  if(CLANG_LEAK_SANITIZER)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")
    message(STATUS "Clang: Thread LeakSanitizer enabled")
  endif()
  if(CLANG_THREAD_SAFETY_ANALYSIS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wthread-safety")	
    message(STATUS "Clang: Thread safety analysis enabled")
  endif()
endif()

# -Wno-narrowing needed to suppress a warning in g3d
# -Wno-deprecated-register is needed to suppress 185 gsoap warnings on Unix systems.
# -Wno-switch because I find this warning useless
target_compile_options(trinity-compile-option-interface
  INTERFACE
    -Wno-narrowing
    -Wno-deprecated-register
    -Wno-switch)

target_compile_definitions(trinity-compile-option-interface
  INTERFACE
    -DDEBUG=1)
	
if (BUILD_SHARED_LIBS)
  # -fPIC is needed to allow static linking in shared libs.
  target_compile_options(trinity-compile-option-interface
    INTERFACE
        -fPIC)

   # -fvisibility=hidden sets the default visibility to hidden to prevent exporting of all symbols.
   target_compile_options(trinity-hidden-symbols-interface
     INTERFACE
         -fvisibility=hidden)

  # --no-undefined to throw errors when there are undefined symbols
  # (caused through missing TRINITY_*_API macros).
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --no-undefined")

  message(STATUS "Clang: Disallow undefined symbols")
endif()

if(USE_GPERFTOOLS)
#NOTE: When compiling with programs with gcc, that you plan to link
#with libtcmalloc, it's safest to pass in the flags
# -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free
#when compiling.  gcc makes some optimizations assuming it is using its
#own, built-in malloc; that assumption obviously isn't true with
#tcmalloc.  In practice, we haven't seen any problems with this, but
#the expected risk is highest for users who register their own malloc
##hooks with tcmalloc (using gperftools/malloc_hook.h).  The risk is
#lowest for folks who use tcmalloc_minimal (or, of course, who pass in
#the above flags :-) ).
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free")
endif()
