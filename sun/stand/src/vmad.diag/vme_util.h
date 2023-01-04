
static char     sccsidu_h[] = "@(#)vme_util.h 1.1 9/25/86 Copyright Sun Micro";

/***********************  TEST #0  INFORMATION  *******************************/
char *test0_info[] = {
	"0_ TEST TIME OUT:                                         ",
	"This test checks for address lines by writing/reading the ",
	"data reg. The data is not checked for reliability.        ",
	"Also, it addresses an address that is not in the range of ",
	"the board to check if time out being happened. Otherwise  ",
	"the error will be reported.				   ",
        0,
};

/***********************  TEST #1  INFORMATION  *******************************/
char *test1_info[] = {
	"1_ TEST DATA PATH:					    ",
        "This test verifies that all data lines from VME to Multibus",
        "and vise versa are O.K.				    ",
        "This test is done by writing bit patterns to the data and  ",
	"counter reg and reading them back for comparison.          ",
	"All 16 bits are tested for the correctness.		    ",
        0,
};


/***********************  TEST #2  INFORMATION  *******************************/
char *test2_info[] = {
	"2_ TEST CONTROL REGISTER :                                 ",
	"This test writes bits in control reg and reads back to compare",
	"to see if right thing happens. Also resets the board and checks",
	"it to see if it reads as 0x1100 or not.                    ",
        0,
};


/***********************  TEST #3  INFORMATION  *******************************/
char *test3_info[] = {
	"3_ TEST INTERRUPT:                                         ",
	"This test checks the functionality of interrupt mechanisim by",
        "interrupting the board and checking the vector being addressed.",
	"In correct case the interrupt vector # 72 must be addressed.",
        0,
};


/***********************  TEST #4  INFORMATION  *******************************/
char *test4_info[] = {
  	"4_ TEST BYTE DMA:                                          ",
	"This routine was designed to check dma circuitry. It does  ",
	"dma 11  bytes of all 0's  one byte at a time which already ",
	"set to all 0xff's. It will check for validity of transfered",
	"byte and also checks for correct byte addressing by examining",
	"the bytes just before and after the transaction byte. Then ",
	"if the test was successful it transfers 256 bytes of all   ", 
	"1's all by one dma transfer. In case of error, you can try ",
	"different patterns by using the tool program.              ", 
        0,

};


/***********************  TEST #5  INFORMATION  *******************************/char *test5_info[] = {
        "5_ TEST WORD DMA:                                          ",
        "this test will check the DMA circuitry in word mode by     ",
        "writing 256 word of all 0's to the area of memory that has ",
        "been set to all 1's. All 256 word will be transferred at   ",
        "the same time. Then the it will check for the value other  ",
        "than 0's. If the test fails, you can try different patterns",
        "by using the tool program                                  ",
        0,
};


/***********************  TEST #6  INFORMATION  *******************************/char *test6_info[] = {
        "0_ TEST TIME OUT:                                         ",
        "This test checks for address lines by writing/reading the ",
        "data reg. The data is not checked for reliability.        ",
        "Also, it addresses an address that is not in the range of ",
        "the board to check if time out being happened. Otherwise  ",
        "the error will be reported.                               ",
        "1_ TEST DATA PATH:                                         ",
        "This test verifies that all data lines from VME to Multibus",
        "and vise versa are O.K.                                    ",
        "This test is done by writing bit patterns to the data and  ",
        "counter reg and reading them back for comparison.          ",
        "All 16 bits are tested for the correctness.                ",
        "2_ TEST CONTROL REGISTER :                                 ",
        "This test writes bits in control reg and reads back to compare",
        "to see if right thing happens. Also resets the board and checks",
        "it to see if it reads as 0x1100 or not.                    ",
        "3_ TEST INTERRUPT:                                         ",
        "This test checks the functionality of interrupt mechanisim by",
        "interrupting the board and checking the vector being addressed.",
        "In correct case the interrupt vector # 72 must be addressed.",
        "4_ TEST BYTE DMA:                                          ",
        "This routine was designed to check dma circuitry. It does  ",
        "dma 11  bytes of all 0's  one byte at a time which already ",
        "set to all 0xff's. It will check for validity of transfered",
        "byte and also checks for correct byte addressing by examining",
        "the bytes just before and after the transaction byte. Then ",
        "if the test was successful it transfers 256 bytes of all   ",
        "1's all by one dma transfer. In case of error, you can try ",
        "different patterns by using the tool program.              ",
        "5_ TEST WORD DMA:                                          ",
        "this test will check the DMA circuitry in word mode by     ",
        "writing 256 word of all 0's to the area of memory that has ",
        "been set to all 1's. All 256 word will be transferred at   ",
        "the same time. Then the it will check for the value other  ",
        "than 0's. If the test fails, you can try different patterns",
        "by using the tool program                                  ",
        0,
};


char **test_info[] = {
	test0_info,
        test1_info,
        test2_info,
        test3_info,
        test4_info,
        test5_info,
	test6_info,
        0, 
};



char *debug0_info[] = {
	"This debug tool will loop on accessing the data register. The routine",
	"will catch bus errors and will not time out. User can abort this ",
	"debug loop by entering the conrol key. ",
	0,
};

char *debug1_info[] = {
	"This debug tool will loop on reseting the board. Bus errors will",
	"not cause the program to abort abnormally. User can stop the loop ",
	"by entering the conrol key.",
	0,
};

char *debug2_info[] = {
	"This debug loop will continuosly tries to interrupt the board. The",
	"interrupt service routine will do nothing but returning to the ",
	"caller. User can stop the loop by entering the control key.",
	0,
};

char *debug3_info[] = {
	"This debug loop will contineuosly transfers the user given amount of",
	"bytes/words until user stops the loop by a control key. Each time it",
	"will dma for the number of bytes/words and starts over and over. The",
	"user has the oppotinuty of checking the read/write dma strobes ",
	"and the data/address lines as well.",
	0,
};

char *debug5_info[] = {
	"This routine is used to verify data register by writing a ",
	"user given pattern to the register and then reading it back to",
	"compare for a number of times given by user. User can terminate",
	"the loop by a control key.",
	0,
};

char *debug6_info[] = {
        "This routine is used to verify count register by writing a ",
        "user given pattern to the register and then reading it back to",
        "comparing for a number of times given by user. User can terminate",
        "the loop by a control key.",
        0,
};

char **debug_info[] = {
	debug0_info,
	debug1_info,
	debug2_info,
	debug3_info,
	debug3_info,
	debug5_info,
	debug6_info,
	0,
};
