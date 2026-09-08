// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#include <nyx/utils.h>
#include <nyx/evaluation.h>

int
evaluate(position *p)
{
	return p->sf->material * white_black(+1, -1, p->stm);
}
