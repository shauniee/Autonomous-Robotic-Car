ifeq ($(TEST),1)
S1_TEST_SRCS := $(wildcard ../testing/subsystem1/*.c)
S1_TEST_OBJS := $(patsubst ../%.c,mtkernel_3/%.o,$(S1_TEST_SRCS))
OBJS += $(S1_TEST_OBJS)
C_DEPS += $(S1_TEST_OBJS:.o=.d)
mtkernel_3/testing/subsystem1/%.o: ../testing/subsystem1/%.c
	@mkdir -p $(dir $@)
	$(GCC) $(CFLAGS) -D$(TARGET) $(INCPATH) -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
endif
