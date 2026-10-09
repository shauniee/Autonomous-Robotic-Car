# S3 firmware tests are opt-in; production profiles compile no test fixtures.
ifeq ($(TEST),1)
S3_TEST_SRCS := $(wildcard ../testing/subsystem3/*.c)
S3_TEST_OBJS := $(patsubst ../%.c,mtkernel_3/%.o,$(S3_TEST_SRCS))
OBJS += $(S3_TEST_OBJS)
C_DEPS += $(S3_TEST_OBJS:.o=.d)

mtkernel_3/testing/subsystem3/%.o: ../testing/subsystem3/%.c
	@echo 'Building file: $<'
	@mkdir -p $(dir $@)
	$(GCC) $(CFLAGS) -D$(TARGET) $(INCPATH) -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
endif
