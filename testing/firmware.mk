# Active firmware fixtures live here and are linked only in TEST builds.
ifeq ($(TEST),1)
# Firmware fixtures only. The pure/ host runner sources own main() and must
# never be compiled into the RTOS image.
TEST_SRCS := $(wildcard ../testing/subsystem1/*.c \
	../testing/subsystem2/*.c \
	../testing/subsystem3/*.c \
	../testing/subsystem4/*.c \
	../testing/subsystem5/*.c \
	../testing/integration/*.c)
TEST_OBJS := $(patsubst ../testing/%.c,mtkernel_3/testing/%.o,$(TEST_SRCS))
OBJS += $(TEST_OBJS)
C_DEPS += $(TEST_OBJS:.o=.d)
mtkernel_3/testing/%.o: ../testing/%.c
	@mkdir -p $(dir $@)
	$(GCC) $(CFLAGS) -D$(TARGET) $(INCPATH) -MF"$(@:.o=.d)" -MT"$@" -c -o "$@" "$<"
endif
