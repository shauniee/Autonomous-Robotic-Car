# All active fixture sources belong to testing/, linked in TEST only.
ifeq ($(TEST),1)
TEST_SRCS := $(wildcard ../testing/*/*.c)
TEST_OBJS := $(patsubst ../testing/%.c,mtkernel_3/testing/%.o,$(TEST_SRCS))
OBJS += $(TEST_OBJS)
C_DEPS += $(TEST_OBJS:.o=.d)
mtkernel_3/testing/%.o: ../testing/%.c
	@mkdir -p $(dir $@)
	$(GCC) $(CFLAGS) -D$(TARGET) $(INCPATH) -MF"$(@:.o=.d)" -MT"$@" -c -o "$@" "$<"
endif
