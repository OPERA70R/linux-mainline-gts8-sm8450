// SPDX-License-Identifier: GPL-2.0
/*
 * Memory Bandwidth Allocation (MBA) test
 *
 * Copyright (C) 2018 Intel Corporation
 *
 * Authors:
 *    Sai Praneeth Prakhya <sai.praneeth.prakhya@intel.com>,
 *    Fenghua Yu <fenghua.yu@intel.com>
 */
#include "resctrl.h"

#define RESULT_FILE_NAME	"result_mba"
#define NUM_OF_RUNS		5
#define MAX_DIFF_PERCENT	8
#define ALLOCATION_MAX		100
#define ALLOCATION_MIN		10
#define ALLOCATION_STEP		10

/*
 * Change schemata percentage from 100 to 10%. Write schemata to specified
 * con_mon grp, mon_grp in resctrl FS.
 * For each allocation, run 5 times in order to get average values.
 */
static int mba_setup(const struct resctrl_test *test,
		     const struct user_params *uparams,
		     struct resctrl_val_param *p)
{
	static int runs_per_allocation, allocation = 100;
	char allocation_str[64];
	int ret;

	if (runs_per_allocation >= NUM_OF_RUNS)
		runs_per_allocation = 0;

	/* Only set up schemata once every NUM_OF_RUNS of allocations */
	if (runs_per_allocation++ != 0)
		return 0;

	if (allocation < ALLOCATION_MIN || allocation > ALLOCATION_MAX)
		return END_OF_TESTS;

	sprintf(allocation_str, "%d", allocation);

	ret = write_schemata(p->ctrlgrp, allocation_str, uparams->cpu, test->resource);
	if (ret < 0)
		return ret;

	allocation -= ALLOCATION_STEP;

	return 0;
}

static bool show_mba_info(unsigned long *bw_imc, unsigned long *bw_resc)
{
	int allocation, runs;
	bool ret = false;

	ksft_print_msg("Results are displayed in (MB)\n");
	/* Memory bandwidth from 100% down to 10% */
	for (allocation = 0; allocation < ALLOCATION_MAX / ALLOCATION_STEP;
	     allocation++) {
		unsigned long avg_bw_imc, avg_bw_resc;
		unsigned long sum_bw_imc = 0, sum_bw_resc = 0;
		int avg_diff_per;
		float avg_diff;

		/*
		 * The first run is discarded due to inaccurate value from
		 * phase transition.
		 */
		for (runs = NUM_OF_RUNS * allocation + 1;
		     runs < NUM_OF_RUNS * allocation + NUM_OF_RUNS ; runs++) {
			sum_bw_imc += bw_imc[runs];
			sum_bw_resc += bw_resc[runs];
		}

		avg_bw_imc = sum_bw_imc / (NUM_OF_RUNS - 1);
		avg_bw_resc = sum_bw_resc / (NUM_OF_RUNS - 1);
		avg_diff = (float)labs(avg_bw_resc - avg_bw_imc) / avg_bw_imc;
		avg_diff_per = (int)(avg_diff * 100);

		ksft_print_msg("%s Check MBA diff within %d%% for schemata %u\n",
			       avg_diff_per > MAX_DIFF_PERCENT ?
			       "Fail:" : "Pass:",
			       MAX_DIFF_PERCENT,
			       ALLOCATION_MAX - ALLOCATION_STEP * allocation);

		ksft_print_msg("avg_diff_per: %d%%\n", avg_diff_per);
		ksft_print_msg("avg_bw_imc: %lu\n", avg_bw_imc);
		ksft_print_msg("avg_bw_resc: %lu\n", avg_bw_resc);
		if (avg_diff_per > MAX_DIFF_PERCENT)
			ret = true;
	}

	ksft_print_msg("%s Check schemata change using MBA\n",
		       ret ? "Fail:" : "Pass:");
	if (ret)
		ksft_print_msg("At least one test failed\n");

	return ret;
}

static int check_results(void)
{
	char *token_array[8], output[] = RESULT_FILE_NAME, temp[512];
	unsigned long bw_imc[1024], bw_resc[1024];
	int runs;
	FILE *fp;

	fp = fopen(output, "r");
	if (!fp) {
		ksft_perror(output);

		return -1;
	}

	runs = 0;
	while (fgets(temp, sizeof(temp), fp)) {
		char *token = strtok(temp, ":\t");
		int fields = 0;

		while (token) {
			token_array[fields++] = token;
			token = strtok(NULL, ":\t");
		}

		/* Field 3 is perf imc value */
		bw_imc[runs] = strtoul(token_array[3], NULL, 0);
		/* Field 5 is resctrl value */
		bw_resc[runs] = strtoul(token_array[5], NULL, 0);
		runs++;
	}

	fclose(fp);

	return show_mba_info(bw_imc, bw_resc);
}

static void mba_test_cleanup(void)
{
	remove(RESULT_FILE_NAME);
}

static int mba_run_test(const struct resctrl_test *test, const struct user_params *uparams)
{
	struct resctrl_val_param param = {
		.resctrl_val	= MBA_STR,
		.ctrlgrp	= "c1",
		.mongrp		= "m1",
		.filename	= RESULT_FILE_NAME,
		.bw_report	= "reads",
		.setup		= mba_setup
	};
	int ret;

	remove(RESULT_FILE_NAME);

	ret = resctrl_val(test, uparams, uparams->benchmark_cmd, &param);
	if (ret)
		return ret;

	ret = check_results();

	return ret;
}

static bool mba_feature_check(const struct resctrl_test *test)
{
	return test_resource_feature_check(test) &&
	       resctrl_mon_feature_exists("L3_MON", "mbm_local_bytes");
}

struct resctrl_test mba_test = {
	.name = "MBA",
	.resource = "MB",
	.vendor_specific = ARCH_INTEL,
	.feature_check = mba_feature_check,
	.run_test = mba_run_test,
	.cleanup = mba_test_cleanup,
};
