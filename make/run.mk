COMPILE_COMMANDS=$(CURDIR)/compile_commands.json

$(COMPILE_COMMANDS):
	compiledb make clean all

run-clang-format:
	find $(CURDIR) | grep "\.[h|c]$$" | xargs -n 1 clang-format -n

run-clang-tidy: $(COMPILE_COMMANDS)
	find $(CURDIR) | grep "\.[h|c]$$" | xargs -n 1 clang-tidy --warnings-as-errors -checks='clang-diagnostic-*,clang-analyzer-*,' -p $^

run-cppcheck: $(COMPILE_COMMANDS)
	find $(CURDIR) | grep "\.[h|c]$$" | xargs -n 1 cppcheck --enable=all --std=c11 --suppress=missingIncludeSystem --project=$^

.PHONY: \
	run-clang-format \
	run-clang-tidy \
	run-cppcheck
