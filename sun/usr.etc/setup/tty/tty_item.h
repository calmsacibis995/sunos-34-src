/*	@(#)tty_item.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* 
 * Handy dandy macros for creating items:
 */
#define TEXT_ITEM(form, row, col, label, i_col, field_size)	\
form_item_create(form, 					\
		 FORM_ITEM_TEXT, 			\
		 FORM_INPUT_OUTPUT_ITEM, 		\
		 ITEM_ROW, 	row, 			\
		 ITEM_COL, 	col, 			\
		 ITEM_LABEL, 	label, 			\
		 ITEM_VALUE_COL, i_col, 			\
		 ITEM_TEXT_FIELD_SIZE, field_size, 	\
		 0);

#define TEXT_ITEM_OO(form, row, col, label, i_col, field_size)	\
form_item_create(form, 					\
		 FORM_ITEM_TEXT, 			\
		 FORM_OUTPUT_ONLY_ITEM, 		\
		 ITEM_ROW, 	row, 			\
		 ITEM_COL, 	col, 			\
		 ITEM_LABEL, 	label, 			\
		 ITEM_VALUE_COL, i_col, 		\
		 ITEM_TEXT_FIELD_SIZE, field_size, 	\
		 0);


#define CHOICE_ITEM(form, row, col, label, i_col)	\
form_item_create(form, 					\
		 FORM_ITEM_CHOICE, 			\
		 FORM_INPUT_OUTPUT_ITEM, 		\
		 ITEM_ROW, 	row, 			\
		 ITEM_COL, 	col, 			\
		 ITEM_VALUE_COL, i_col, 		\
		 ITEM_LABEL, 	label, 			\
		 0);


#define BUTTON_ITEM(form, row, col, label)		\
form_item_create(form, 					\
		 FORM_ITEM_BUTTON, 			\
		 FORM_INPUT_OUTPUT_ITEM, 		\
		 ITEM_ROW, 	row, 			\
		 ITEM_COL, 	col, 			\
		 ITEM_LABEL, 	label, 			\
		 0);



#define TOGGLE_ITEM(form, row, col, label, i_col)	\
form_item_create(form, 					\
		 FORM_ITEM_TOGGLE, 			\
		 FORM_INPUT_OUTPUT_ITEM, 		\
		 ITEM_ROW, 	row, 			\
		 ITEM_COL, 	col, 			\
		 ITEM_LABEL, 	label, 			\
		 ITEM_VALUE_COL, i_col, 			\
		 0);



#define MSG_ITEM(form, row, col, label)			\
form_item_create(form, 					\
		 FORM_ITEM_BUTTON, 			\
		 FORM_OUTPUT_ONLY_ITEM, 		\
		 ITEM_ROW, 	row, 			\
		 ITEM_COL, 	col, 			\
		 ITEM_LABEL, 	label, 			\
		 0);


