#ifndef lint
static char sccsid[] = "@(#)ndctowld77.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

int map_ndc_to_world_2();
int map_ndc_to_world_3();
int map_world_to_ndc_2();
int map_world_to_ndc_3();

int mapndctoworld2_(ndcx, ndcy, wldx, wldy)
float *ndcx, *ndcy, *wldx, *wldy;
	{
	return(map_ndc_to_world_2(*ndcx, *ndcy, wldx, wldy));
	}

int mapndctoworld3_(ndcx, ndcy, ndcz, wldx, wldy, wldz)
float *ndcx, *ndcy, *ndcz, *wldx, *wldy, *wldz;
	{
	return(map_ndc_to_world_3(*ndcx, *ndcy, *ndcz, wldx, wldy, wldz));
	}

int mapworldtondc2_(wldx, wldy, ndcx, ndcy)
float *wldx, *wldy, *ndcx, *ndcy;
	{
	return(map_world_to_ndc_2(*wldx, *wldy, ndcx, ndcy));
	}

int mapworldtondc3_(wldx, wldy, wldz, ndcx, ndcy, ndcz)
float *wldx, *wldy, *wldz, *ndcx, *ndcy, *ndcz;
	{
	return(map_world_to_ndc_3(*wldx, *wldy, *wldz, ndcx, ndcy, ndcz));
	}
