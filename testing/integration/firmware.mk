# Integration fixtures compile only in motor-inhibited TEST profiles.
ifeq ($(TEST),1)
INTEGRATION_TEST_SRCS := $(wildcard ../testing/integration/*.c)
INTEGRATION_TEST_OBJS := $(patsubst ../%.c,mtkernel_3/%.o,$(INTEGRATION_TEST_SRCS))
OBJS += $(INTEGRATION_TEST_OBJS)
C_DEPS += $(INTEGRATION_TEST_OBJS:.o=.d)

mtkernel_3/testing/integration/%.o: ../testing/integration/%.c
	@mkdir -p $(dir $@)
	$(GCC) $(CFLAGS) -D$(TARGET) $(INCPATH) -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
endif
