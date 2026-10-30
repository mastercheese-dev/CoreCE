#!/usr/bin/env python3
"""The desktop updater's choice of a main or testing release."""

import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "port/linux/src/update_choice.c"

DRIVER = r"""
#include "update_choice.h"
#include <stdio.h>
#include <string.h>

static int expect(const char *name, int found, struct update_choice *choice, long number, const char *tag)
{
	int ok = found == (number != 0) && (!found || (choice->number == number && !strcmp(choice->tag, tag)));
	if (!ok)
		fprintf(stderr, "%s: found %d number %ld tag %s, wanted %ld %s\n",
			name, found, found ? choice->number : 0, found ? choice->tag : "", number, tag ? tag : "");
	return ok;
}

int main(void)
{
	const char *list =
		"[{"
		"\"tag_name\":\"build-28\",\"draft\":false,\"prerelease\":false,"
		"\"body\":\"notes mention \\\"tag_name\\\": \\\"build-999\\\" and {braces}\","
		"\"assets\":[{\"name\":\"halo-windows-release.zip\"},{\"name\":\"halo-linux-release.zip\"}]"
		"},{"
		"\"tag_name\":\"testing-4\",\"draft\":false,\"prerelease\":true,"
		"\"assets\":[{\"name\":\"halo-windows-release.zip\"},{\"name\":\"CoreCE-macos-arm64.dmg\"}]"
		"},{"
		"\"tag_name\":\"testing-9\",\"draft\":false,\"prerelease\":false,"
		"\"assets\":[{\"name\":\"halo-windows-release.zip\"}]"
		"},{"
		"\"tag_name\":\"testing-2\",\"draft\":false,\"prerelease\":true,"
		"\"assets\":[{\"name\":\"halo-windows-release.zip\"}]"
		"},{"
		"\"tag_name\":\"build-70\",\"draft\":false,\"prerelease\":false,"
		"\"assets\":[{\"name\":\"CoreCE-macos-arm64.dmg\"}]"
		"},{"
		"\"tag_name\":\"build-3\",\"draft\":true,\"prerelease\":false,"
		"\"assets\":[{\"name\":\"halo-windows-release.zip\"}]"
		"}]";
	const char *latest =
		"{\"tag_name\":\"build-30\",\"draft\":false,\"prerelease\":false,"
		"\"assets\":[{\"name\":\"halo-windows-release.zip\"}]}";
	struct update_choice choice;
	int failed = 0;

	failed += !expect("older main stays", update_choice_select(list, NULL, "halo-windows-release.zip", 0, 0, 30, &choice),
		&choice, 0, NULL);
	failed += !expect("main newer from latest", update_choice_select(list, latest, "halo-windows-release.zip", 0, 0, 28, &choice),
		&choice, 30, "build-30");
	failed += !expect("main current", update_choice_select(list, latest, "halo-windows-release.zip", 0, 0, 30, &choice),
		&choice, 0, NULL);
	failed += !expect("leave testing", update_choice_select(list, latest, "halo-windows-release.zip", 0, 1, 80, &choice),
		&choice, 30, "build-30");
	failed += !expect("enter testing", update_choice_select(list, latest, "halo-windows-release.zip", 1, 0, 70, &choice),
		&choice, 4, "testing-4");
	failed += !expect("testing newer", update_choice_select(list, latest, "halo-windows-release.zip", 1, 1, 2, &choice),
		&choice, 4, "testing-4");
	failed += !expect("testing current", update_choice_select(list, latest, "halo-windows-release.zip", 1, 1, 4, &choice),
		&choice, 0, NULL);
	failed += !expect("linux asset", update_choice_select(list, NULL, "halo-linux-release.zip", 0, 0, 1, &choice),
		&choice, 28, "build-28");
	failed += !expect("missing asset", update_choice_select(list, NULL, "halo-android-release.zip", 0, 0, 1, &choice),
		&choice, 0, NULL);
	return failed ? 1 : 0;
}
"""


class UpdateChoiceTest(unittest.TestCase):
    def test_channel_rules(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            driver = directory / "driver.c"
            driver.write_text(DRIVER)
            binary = directory / "choice"
            subprocess.run(
                ["clang", "-Wall", "-Wextra", "-Werror", "-I", str(SOURCE.parent),
                 str(SOURCE), str(driver), "-o", str(binary)],
                check=True)
            subprocess.run([binary], check=True)


if __name__ == "__main__":
    unittest.main()
