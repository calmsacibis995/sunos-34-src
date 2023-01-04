static  char    sccsid_uh[] = "@(#)sc_util.h 1.1 86/09/25 SMI." ;

/***********************  TEST #1  INFORMATION  *******************************/
char *test1_info[] = {
	"This test writes test patterns from  0x00  to  0x3F into",
	"ICR and the results are read back and compared with the",
	"expected values. It also checks for time out on read and",
        "write accesses to ICR.",
        0,	
};

/***********************  TEST #2  INFORMATION  *******************************/
char *test2_info[] = {
        "This test writes test patterns from  0x0000 to 0xFFFF into",
	"DMA counter register  then  the results are read back  and",
	"compared with the expected values.",
        0, 
};

/***********************  TEST #3  INFORMATION  *******************************/
char *test3_info[] = {
        "This test writes test patterns from  0x000000 to 0xFFFFFF",
	"into DMA address register then  the results are read back",
        "and compared with the expected values.",
        0, 
};

/***********************  TEST #4  INFORMATION  *******************************/
char *test4_info[] = {
  	"This test verifies device initial selection sequence by:",
	"first setting the ID field on data register and asserts",
    	"the select bit in ICR ( bit  5 ), then check for a busy",
	"response from selected device.",
	0,
};

/***********************  TEST #5  INFORMATION  *******************************/
char *test5_info[] = {
        "This test verifies bus operation and device's status.",
	"This is done  by issuing  Test Unit Ready and Request ",
        "sense commands to device. Status and sense information",
	"are read from device and checked for mornal operations.",
        0,
};

/***********************  TEST #6  INFORMATION  *******************************/
char *test6_info[] = {
        "This test verifies bus transfer operations. This is done",
        "with  WRITE  and  READ  operations of  512 bytes of data",
        "patterns to disk. Both operations are done by program I/O.", 
  	"The results are  read back and compared with the expected ",
        "values.",
      	0,
};

/***********************  TEST #7  INFORMATION  *******************************/
char *test7_info[] = {
	"This test verifies DMA transfer operation. This is done",
	"by enabling DMA and WORD mode, then do a READ on 512 bytes.",
	"At commpletion of DMA transfer, device should come back and",
	"do a interrupt request for status in. The status and message",
        "are read back and check for any error indications.",
	0,
};

/***********************  TEST #8  INFORMATION  *******************************/
char *test8_info[] = {
	"This test is devided into two parts: The first one does",
	"write and read to interrupt vector register then compares",
	"the results with expected values. This part of the test",
 	"is executed if the board under test is a VME SCSI board.",
	"The second part of the test verifies that request for",
	"status from target device causes an interrupt. This is",
	"done by enabling DMA and WORD mode, do DMA transfer on",
	"request sense, then verifies that interrupt occurred.",
	0,
};

/***********************  TEST #9  INFORMATION  *******************************/
char *test9_info[] = {
	"This test verifies DMA overrun condition. This is done by",
    	"issuing a Read command, with block count set to 2 (2X512 ",
	"bytes) and DMA counter set to 512. At completion of DMA",
	"transfer of the 512th byte, the DMA counter will generates",
	"an overrun signal which will disable any further DMA. The",
	"DMA overrun signal will cause an interrupt request.",
	0,
};

/***********************  TEST #10  INFORMATION *******************************/
char *test10_info[] = {
	"This test writes 10K block of random data to disk device,",
	"reads them back (using DMA transfers) and verifies that data",
        "transfered between disk device and main memory are OK.",
	0,
};

/***********************  TEST #11  INFORMATION *******************************/
char *test11_info[] = {
	"Not implemented yet...",
	0,
};

/***********************  TEST #12  INFORMATION *******************************/
char *test12_info[] = {
	0,
};

char **test_info[] = {
        0,
        test1_info,
        test2_info,
        test3_info,
        test4_info,
        test5_info,
        test6_info,
        test7_info,
        test8_info,
        test9_info,
        test10_info,
        test11_info,
        test12_info,
        0,
};

