ifeq ($(TEST),1)
S2_TEST_SRCS := $(wildcard ../testing/subsystem2/*.c)
S2_TEST_OBJS := $(patsubst ../%.c,mtkernel_3/%.o,$(S2_TEST_SRCS))
OBJS += $(S2_TEST_OBJS)
C_DEPS += $(S2_TEST_OBJS:.o=.d)
mtkernel_3/testing/subsystem2/%.o: ../testing/subsystem2/%.c
	@mkdir -p $(dir $@)
	$(GCC) $(CFLAGS) -D$(TARGET) $(INCPATH) -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
endif
