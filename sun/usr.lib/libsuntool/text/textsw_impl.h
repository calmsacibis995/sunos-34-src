/*      @(#)textsw_impl.h 1.4 87/01/07 SMI      */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#ifndef textsw_impl_DEFINED
#define textsw_impl_DEFINED
/*
 * Internal structures for textsw implementation.
 */

#				ifndef timerclear
#include <sys/time.h>
#				endif
#				ifndef NOTIFY_DEFINED
#include <sunwindow/notify.h>
#				endif
#				ifndef scrollbar_DEFINED
#include <suntool/scrollbar.h>
#				endif
#				ifndef suntool_entity_view_DEFINED
#include "entity_view.h"
#				endif
#				ifndef suntool_selection_svc_DEFINED
#include <suntool/selection_svc.h>
#				endif
#				ifndef suntool_selection_attributes_DEFINED
#include <suntool/selection_attributes.h>
#				endif
#				ifndef suntool_textsw_DEFINED
#include <suntool/textsw.h>
#				endif

/* Although not needed to support types used in this header file, the
 * following are needed to support macros defined here.
 */
#				ifndef sunwindow_win_input_DEFINED
#include <sunwindow/win_input.h>
#				endif

#ifndef	NULL
#define	NULL	0
#endif

#define TXTSW_DO_AGAIN(_textsw)	\
	((_textsw->again_count != 0) && \
	 ((_textsw->state & TXTSW_NO_AGAIN_RECORDING) == 0))
#define TXTSW_DO_UNDO(_textsw)	(_textsw->undo_count != 0)

typedef struct textsw_string {
	int	max_length;
	char	*base;
	char	*free;
} string_t;
pkg_private string_t	null_string;
#define TXTSW_STRING_IS_NULL(ptr_to_string_t)				\
	((ptr_to_string_t)->base == null_string.base)
#define TXTSW_STRING_BASE(ptr_to_string_t)				\
	((ptr_to_string_t)->base)
#define TXTSW_STRING_FREE(ptr_to_string_t)				\
	((ptr_to_string_t)->free)
#define TXTSW_STRING_LENGTH(ptr_to_string_t)				\
	(TXTSW_STRING_FREE(ptr_to_string_t)-TXTSW_STRING_BASE(ptr_to_string_t))

typedef struct key_map_object {
	struct key_map_object	*next;
	short			 event_code;
	short unsigned		 type;
	short			 shifts;
	caddr_t			 maps_to;
} Key_map_object;
typedef Key_map_object	*Key_map_handle;
#define	TXTSW_KEY_FILTER	0
	/* maps_to is argv => char[][] */
#define	TXTSW_KEY_SMART_FILTER	1
	/* maps_to is argv => char[][] */
#define	TXTSW_KEY_MACRO		2
	/* maps_to is again script => string_t */
#define	TXTSW_KEY_NULL		32767


typedef struct textsw_selection_object {
	long unsigned		  type;
	Es_index		  first, last_plus_one;
	char			 *buf;
	int			  buf_len, buf_max_len;
	int			  buf_is_dynamic;
	Seln_result		(*per_buffer)();
	char			 *per_buffer_context;
} Textsw_selection_object;
/*	type includes modes, etc (contains return from textsw_func_selection,
 *	  see below)
 *	first == last_plus_one == ES_INFINITY when object is initialized.
 *	per_buffer == NULL when object is initialized, and should be set iff
 *	  long non-local selections are to be processed.  In this case, the
 *	  per_buffer_context slot is available to store the current program
 *	  context to the per_buffer routine.
 */
typedef Textsw_selection_object	*Textsw_selection_handle;
	/* Flags for textsw_func_selection */
#define	TFS_FILL_IF_OTHER	1
#define	TFS_FILL_IF_SELF	2
#define	TFS_FILL_ALWAYS		(TFS_FILL_IF_OTHER|TFS_FILL_IF_SELF)
	/* Return values for textsw_func_selection */
#define	TFS_ERROR		 EV_SEL_CLIENT_FLAG(0x4000)
#define	TFS_IS_ERROR(arg)	(arg & TFS_ERROR)
#define	TFS_SELN_SVC_ERROR	(TFS_ERROR|1)
#define	TFS_WARNING		 EV_SEL_CLIENT_FLAG(0x8000)
#define	TFS_IS_WARNING(arg)	(arg & TFS_WARNING)
#define	TFS_BAD_ATTR_WARNING	(EV_SEL_CLIENT_FLAG(0x0100)|TFS_WARNING)
#define	TFS_IS_OTHER		 EV_SEL_CLIENT_FLAG(0x0001)
#define	TFS_IS_SELF		 EV_SEL_CLIENT_FLAG(0x0002)

typedef struct textsw_view_object {
	long unsigned			  magic;
	struct textsw_object		 *folio;
	struct textsw_view_object	 *next;
	int				  window_fd;
	Rect				  rect;
	Ev_handle			  e_view;
	Scrollbar			  scrollbar;
	long unsigned			  state;
} Textsw_view_object;
typedef Textsw_view_object *Textsw_view;
#define	TEXTSW_VIEW_NULL	((Textsw_view)0)
#define	TEXTSW_VIEW_MAGIC	0xF0110A0A

#define PIXWIN_FOR_VIEW(_view)		((_view)->e_view->pw)
#define WIN_FD_FOR_VIEW(_view)		((_view)->window_fd)
#define SCROLLBAR_FOR_VIEW(_view)	((_view)->scrollbar)
	/* Bit flags for Textsw_view->state */
	
#define TXTSW_DONT_REDISPLAY	0x10000000
#define TXTSW_UPDATE_SCROLLBAR	0x20000000
#define TXTSW_VIEW_DISPLAYED	0x40000000	
#define TXTSW_VIEW_DYING	0x80000000


#define	TEXTSW_MAGIC		0xF2205050
#define TXTSW_UI_BUFLEN	12
typedef struct textsw_object {
	long unsigned	  magic;
	struct textsw_object
			 *next;
	Textsw_view	  first_view;
	caddr_t		  tool;		/* Actually a (struct tool *) */
	caddr_t		  menu;
	Ev_chain	  views;
	Es_handle	(*es_create)();
	int		(*notify)();
	unsigned	  notify_level;
	char		  to_insert[TXTSW_UI_BUFLEN];
	char		 *to_insert_next_free;
	unsigned	  to_insert_counter;
	Es_handle	  trash;	/* Stuff deleted from piece_source */
	long unsigned	  state,
			  func_state;
	short unsigned	  caret_state,
			  holder_state,
			  track_state;
	int		  multi_click_space,
			  multi_click_timeout;
	Textsw_enum	  insert_makes_visible;
	unsigned	  span_level;
	struct timeval	  last_ie_time,
			  last_adjust,
			  last_point,
			  selection_died,
			  timer;
	short		  last_click_x,
			  last_click_y;
	Es_index	  adjust_pivot;	/* Valid iff TXTSW_TRACK_ADJUST */
	Ev_mark_object	  read_only_boundary;
	Ev_mark_object	  save_insert;	/* Valid iff TXTSW_GET/PUTTING	*/
	unsigned	  again_count;
	unsigned	  undo_count;
	string_t	 *again;
	Es_index	  again_first, again_last_plus_one;
	int		  again_insert_length; /* Offset from again[0]->base */
	caddr_t		 *undo;
	Seln_client	  selection_client;
	Seln_function_buffer
			  selection_func;
	Seln_holder	 *selection_holder;
	short		  func_x,
			  func_y;
	Textsw_view	  func_view;
	Key_map_handle	  key_maps;
	int		  owed_by_filter;
	unsigned	  es_mem_maximum;
	unsigned	  ignore_limit;
	char		  edit_bk_char,
			  edit_bk_word,
			  edit_bk_line;
	caddr_t		  client_data;
	caddr_t		  menu_table;
	
	int		  checkpoint_frequency;
	int		  checkpoint_number;
	char		 *checkpoint_name;
	Textsw_view	  coalesce_with;
} Text_object;
typedef Text_object *	Textsw_folio;
#define	TEXTSW_FOLIO_NULL	((Textsw_folio)0)

#ifdef DEBUG
pkg_private Textsw_folio	textsw_folio_for_view();
pkg_private Textsw_view		textsw_view_abs_to_rep();
pkg_private Textsw		textsw_view_rep_to_abs();
#define	FOLIO_FOR_VIEW(_view)		textsw_folio_for_view(_view)
#define	VIEW_ABS_TO_REP(_public)	textsw_view_abs_to_rep(_public)
#define	VIEW_REP_TO_ABS(_private)	textsw_view_rep_to_abs(_private)
#else
#define	FOLIO_FOR_VIEW(_view)		((_view)->folio)
#define	VIEW_ABS_TO_REP(_public)	((Textsw_view)_public)
#define	VIEW_REP_TO_ABS(_private)	((Textsw)_private)
#endif

#define	IS_FOLIO(_folio)		((_folio)->magic == TEXTSW_MAGIC)
#define	IS_VIEW(_view)			((_view)->magic == TEXTSW_VIEW_MAGIC)
#define	VALIDATE_FOLIO(_folio)		ASSERT(IS_FOLIO(_folio))
#define	VALIDATE_VIEW(_view)		ASSERT(IS_VIEW(_view))

#ifdef lint
#define	VIEW_FROM_FOLIO_OR_VIEW(_folio_or_view)	\
	(Textsw_view)(_folio_or_view ? 0 : 0)
#define	TOOL_FROM_FOLIO(_folio)	\
	(Tool *)((_folio)->tool ? 0 : 0)
#define	WINDOW_FROM_VIEW(_view)	\
	(Window)((_view) ? 0 : 0)
#else
#define	VIEW_FROM_FOLIO_OR_VIEW(_folio_or_view)				\
	(IS_VIEW((Textsw_view)_folio_or_view)				\
	 ? (Textsw_view)_folio_or_view					\
	 : ((Textsw_folio)_folio_or_view)->first_view)
#define	TOOL_FROM_FOLIO(_folio)	\
	(Tool *)((_folio)->tool)
#define	WINDOW_FROM_VIEW(_view)	\
	(Window)(_view)
#endif

#define	FORALL_TEXT_VIEWS(_folio, _view)			\
	for (_view = (_folio)->first_view; (_view); _view = _view->next)

#define	TXTSW_HAS_READ_ONLY_BOUNDARY(folio_formal)	\
	(!EV_MARK_IS_NULL(&folio_formal->read_only_boundary))

	/* Synonyms for input event codes */
#define TXTSW_ADJUST		MS_MIDDLE
#define TXTSW_MENU		MS_RIGHT
#define TXTSW_POINT		MS_LEFT
#define TXTSW_AGAIN		KEY_LEFT(2)
#define TXTSW_DELETE		KEY_LEFT(10)
#define TXTSW_DELETE_ALT	004
#define TXTSW_FIND		KEY_LEFT(9)
#define TXTSW_FIND_ALT		006
#define TXTSW_GET		KEY_LEFT(8)
#define TXTSW_LOAD_FILE		033
#define TXTSW_PUT		KEY_LEFT(6)
#define TXTSW_PASTE_ALT		007
#define TXTSW_PUT_THEN_GET	020
#define TXTSW_UNDO		KEY_LEFT(4)
#define TXTSW_STOP		WIN_STOP
#define TXTSW_TOP		KEY_LEFT(5)
#define TXTSW_OPEN		KEY_LEFT(7)
#ifdef VT_100_HACK
#define TXTSW_UNDO_ALT		KEY_TOP(1)
#define TXTSW_AGAIN_ALT		KEY_TOP(2)
#else
#define TXTSW_CAPS_LOCK		KEY_TOP(1)
#endif

	/* Bit flags for Textsw_handle->state */
#define TXTSW_AGAIN_HAS_FIND	0x00000001
#define TXTSW_ADJUST_IS_PD	0x00000010
#define TXTSW_AUTO_INDENT	0x00000020
#define TXTSW_CONFIRM_OVERWRITE	0x00000040
#define TXTSW_CONTINUOUS_BUBBLE	0x00000080
#define TXTSW_NO_CD		0x00000100
#define TXTSW_NO_LOAD		0x00000200
#define TXTSW_LOAD_CAN_CD	0x00000400
#define TXTSW_STORE_CHANGES_FILE		\
				0x00000800
#define TXTSW_READ_ONLY_ESH	0x00001000
#define TXTSW_READ_ONLY_SW	0x00002000
#define TXTSW_STORE_SELF_IS_SAVE		\
				0x00004000
#define TXTSW_RETAINED		0x00008000
#define TXTSW_CAPS_LOCK_ON	0x00010000
#define TXTSW_DISPLAYED		0x00020000
#define TXTSW_EDITED		0x00040000
#define TXTSW_INITIALIZED	0x00080000
#define TXTSW_IN_NOTIFY_PROC	0x00100000
#define TXTSW_DOING_EVENT	0x00200000
#define TXTSW_NO_RESET_TO_SCRATCH		\
				0x00400000
#define TXTSW_NO_AGAIN_RECORDING		\
				0x00800000
#define TXTSW_HAS_FOCUS		0x01000000
#define TXTSW_OPENED_FONT	0x02000000
#define TXTSW_PENDING_DELETE	0x04000000
#define TXTSW_DESTROY_ALL_VIEWS	0x08000000
#define TXTSW_CONTROL_DOWN	0x10000000
#define TXTSW_SHIFT_DOWN	0x20000000

#define TXTSW_MISC_UNUSED	0xc000000e

	/* Bit flags for Textsw_handle->caret_state */
#define TXTSW_CARET_FLASHING	0x0001
#define TXTSW_CARET_ON		0x0002
#define TXTSW_CARET_MUST_SHOW	0x0004
#define	TXTSW_CARET_UNUSED	0xfff8

	/* Bit flags for Textsw_handle->func_state */
#define TXTSW_FUNC_AGAIN	0x00000001
#define TXTSW_FUNC_DELETE	0x00000002
#define TXTSW_FUNC_FILTER	0x00000004
#define TXTSW_FUNC_FIND		0x00000008
#define TXTSW_FUNC_GET		0x00000010
#define TXTSW_FUNC_PUT		0x00000020
#define TXTSW_FUNC_UNDO		0x00000040
#define TXTSW_FUNC_ALL		(TXTSW_FUNC_AGAIN | TXTSW_FUNC_DELETE| \
				 TXTSW_FUNC_FILTER| TXTSW_FUNC_FIND  | \
				 TXTSW_FUNC_GET   | TXTSW_FUNC_PUT   | \
				 TXTSW_FUNC_UNDO)
#define	TXTSW_FUNC_SVC_SAW(flags)	\
				((flags) << 8)
#define	TXTSW_FUNC_SVC_SAW_ALL	TXTSW_FUNC_SVC_SAW(TXTSW_FUNC_ALL)
#define	TXTSW_FUNC_EXECUTE	0x01000000
#define TXTSW_FUNC_SVC_REQUEST	0x10000000
#define	TXTSW_FUNC_SVC_ALL	(TXTSW_FUNC_SVC_SAW_ALL | \
				 TXTSW_FUNC_SVC_REQUEST)
#define TXTSW_FUNCTION_UNUSED	0xeeff8080


	/* Bit flags for Textsw_handle->holder_state */
#define TXTSW_HOLDER_OF_CARET	0x0001
#define TXTSW_HOLDER_OF_PSEL	0x0002
#define TXTSW_HOLDER_OF_SSEL	0x0004
#define TXTSW_HOLDER_OF_SHELF	0x0008
#define TXTSW_HOLDER_OF_ALL	(TXTSW_HOLDER_OF_CARET | \
				 TXTSW_HOLDER_OF_PSEL  | \
				 TXTSW_HOLDER_OF_SSEL  | \
				 TXTSW_HOLDER_OF_SHELF)
#define TXTSW_HOLDER_UNUSED	0xfff0

	/* Bit flags for Textsw_handle->track_state */
#define TXTSW_TRACK_ADJUST	0x0001
#define TXTSW_TRACK_ADJUST_END	0x0002
#define TXTSW_TRACK_POINT	0x0004
#define TXTSW_TRACK_SECONDARY	0x0008
#define TXTSW_TRACK_ALL		(TXTSW_TRACK_ADJUST|TXTSW_TRACK_ADJUST_END|\
				 TXTSW_TRACK_POINT|TXTSW_TRACK_SECONDARY)
#define TXTSW_TRACK_UNUSED	0xfff0

#define	TXTSW_IS_BUSY(textsw)				\
	((textsw->state & TXTSW_PENDING_DELETE) ||	\
	 (textsw->func_state & TXTSW_FUNC_ALL) ||	\
	 (textsw->track_state & TXTSW_TRACK_ALL))

#define	TXTSW_IS_READ_ONLY(textsw)			\
	(textsw->state & (TXTSW_READ_ONLY_ESH | TXTSW_READ_ONLY_SW))

	/* Flags for textsw_flush_caches */
#define TFC_INSERT		0x01
#define TFC_DO_PD		0x02
#define	TFC_SEL			0x04
#define TFC_PD_SEL		(TFC_DO_PD|TFC_SEL)
#define TFC_PD_IFF_INSERT	0x08
		/* Delete selection iff chars will be inserted. */
#define TFC_SEL_IFF_INSERT	0x10
		/* Clear selection iff chars will be inserted. */
#define TFC_IFF_INSERTING	(TFC_PD_IFF_INSERT | TFC_SEL_IFF_INSERT | \
				 TFC_INSERT)
#define TFC_ALL			(TFC_IFF_INSERTING|TFC_PD_SEL)
#define TFC_STD			TFC_ALL

	/* Flags for textsw_find_selection_and_normalize */
/* These flags potentially include an EV_SEL_BASE_TYPE */
#define	TFSAN_BACKWARD		EV_SEL_CLIENT_FLAG(0x0001)
#define	TFSAN_REG_EXP		EV_SEL_CLIENT_FLAG(0x0002)
#define	TFSAN_SHELF_ALSO	EV_SEL_CLIENT_FLAG(0x0004)
#define	TFSAN_TAG		EV_SEL_CLIENT_FLAG(0x0008)

#define SET_TEXTSW_TIMER(_timer_h)					\
	(_timer_h)->tv_sec = 0; (_timer_h)->tv_usec = 500000;
#define TIMER_EXPIRED(timer)						\
	(*timer && ((*timer)->tv_sec == 0) && ((*timer)->tv_usec == 0))
#define SCROLLBAR_ENTER_FEEDBACK	1


pkg_private void
textsw_begin_function( /* textsw, function */ );
#							ifdef notdef
	register Textsw_handle	textsw;
	unsigned		function;
#							endif

pkg_private void
textsw_end_function( /* textsw, function */ );
#							ifdef notdef
	register Textsw_handle	textsw;
	unsigned		function;
#							endif

pkg_private int
textsw_adjust_delete_span( /* folio, first, last_plus_one */ );
#							ifdef notdef
	Textsw_folio	 folio;
	Es_index	*first, *last_plus_one;
#							endif
#define	TXTSW_PE_ADJUSTED	0x10000
#define	TXTSW_PE_EMPTY_INTERVAL	0x20000

pkg_private Es_index
textsw_delete_span( /* folio, first, last_plus_one, flags */ );
#							ifdef notdef
	register Textsw_folio	folio;
	Es_index		first, last_plus_one;
	unsigned		flags;
#							endif
#define	TXTSW_DS_DEFAULT		 EV_SEL_CLIENT_FLAG(0x0)
#define	TXTSW_DS_ADJUST			 EV_SEL_CLIENT_FLAG(0x1)
#define	TXTSW_DS_CLEAR_IF_ADJUST(sel)	(EV_SEL_CLIENT_FLAG(0x2)|(sel))
#define	TXTSW_DS_SHELVE			 EV_SEL_CLIENT_FLAG(0x4)
#define	TXTSW_DS_RECORD			 EV_SEL_CLIENT_FLAG(0x8)

pkg_private Es_index
textsw_do_input( /* view, buf, buf_len */ );
#							ifdef notdef
	Textsw_view		 view;
	char			*buf;
	long int		 buf_len;
#							endif

pkg_private Es_index
textsw_input_after( /* view, old_insert_pos, old_length, record */ );
#							ifdef notdef
	Textsw_view		view;
	Es_index		old_insert_pos, old_length;
	int			record;
#							endif

pkg_private Es_index
textsw_do_pending_delete( /* view, type, flags */ );
#							ifdef notdef
	Textsw_view		view;
	unsigned		type;
	int			flags;
#							endif

pkg_private int
textsw_normalize_internal( /*
	view, first, last_plus_one, upper_context, lower_context, flags */ );
#							ifdef notdef
        register Textsw_view	view;
	Es_index		first, last_plus_one;
	int			upper_context, lower_context;
	register unsigned	flags;
#							endif
#define	TXTSW_NI_DEFAULT		 EV_SEL_CLIENT_FLAG(0x0)
#define	TXTSW_NI_AT_BOTTOM		 EV_SEL_CLIENT_FLAG(0x1)
#define	TXTSW_NI_MARK			 EV_SEL_CLIENT_FLAG(0x2)
#define	TXTSW_NI_NOT_IF_IN_VIEW		 EV_SEL_CLIENT_FLAG(0x4)
#define	TXTSW_NI_SELECT(sel)		(EV_SEL_CLIENT_FLAG(0x8)|(sel))

pkg_private Es_index
textsw_set_insert( /* textsw, pos */ );
#							ifdef notdef
	Textsw_folio	textsw;
	Es_index	pos;
#							endif

pkg_private void
textsw_add_timer( /* textsw, timeout */ );
#							ifdef notdef
	register Textsw_folio	 textsw;
	register struct timeval	*timeout;
#							endif

pkg_private Notify_value
textsw_timer_expired( /* textsw, which */ );
#							ifdef notdef
	register Textsw_folio	 textsw;
	int			 which;
#							endif

pkg_private void
textsw_remove_timer( /* textsw */ );
#							ifdef notdef
	register Textsw_handle	textsw;
#							endif

pkg_private void
textsw_invert_caret( /* textsw */ );
#							ifdef notdef
	register Textsw_handle	textsw;
#							endif

pkg_private void
textsw_take_down_caret( /* textsw */ );
#							ifdef notdef
	register Textsw_handle	textsw;
#							endif

pkg_private void
textsw_may_win_exit( /* textsw */ );
#							ifdef notdef
	Textsw_handle	  textsw;
#							endif

pkg_private void
textsw_notify( /* folio_or_view, attributes */ );
#							ifdef notdef
	Textsw_opaque	folio_or_view;
	Attr_avlist	attributes;
#							endif

pkg_private void
textsw_post_error( /* folio_or_view, locx, locy, msg1, msg2 */ );
#							ifdef notdef
	Textsw_opaque	 folio_or_view;
	int		 locx, locy;
	char		*msg1, *msg2;
#							endif

pkg_private void
textsw_read_only_msg( /* textsw, locx, locy */ );
#							ifdef notdef
	Textsw_folio	textsw;
	int		locx, locy;
#							endif

pkg_private Textsw_status
textsw_set_internal( /* view, attrs */ );
#							ifdef notdef
	Textsw_view		view;
	Attr_avlist		attrs;
#							endif
#define	TEXTSW_CONSUME_ATTRS	TEXTSW_ATTR(ATTR_BOOLEAN, 240)

pkg_private Es_status
textsw_checkpoint( /* folio */ );
#							ifdef notdef
	Text_folio		folio;
#							endif

extern caddr_t
textsw_checkpoint_undo( /* abstract, undo_mark */ );
#							ifdef notdef
	Textsw			abstract;
	caddr_t			undo_mark;
#							endif

extern void
textsw_display( /* abstract */ );
#							ifdef notdef
	Textsw			abstract;
#							endif

extern void
textsw_display_view( /* abstract, rect */ );
#							ifdef notdef
	Textsw			 abstract;
	register Rect		*rect;
#							endif

pkg_private void
textsw_display_view_margins( /* abstract, rect */ );
#							ifdef notdef
	Textsw			 abstract;
	register Rect		*rect;
#							endif

pkg_private int
textsw_is_seln_nonzero( /* textsw, type */ );
#							ifdef notdef
	register Text_folio	textsw;
	unsigned		type;
#							endif

pkg_private Es_index
textsw_find_mark_internal( /* textsw, mark */ );
#							ifdef notdef
	Textsw_folio	textsw;
	Ev_mark_object	mark;
#							endif

pkg_private Es_index
textsw_get_saved_insert( /* textsw */ );
#							ifdef notdef
	register Textsw_folio	textsw;
#							endif

pkg_private Es_index
textsw_read_only_boundary_is_at( /* folio */ );
#							ifdef notdef
	register Textsw_folio	folio;
#							endif

pkg_private Es_index
textsw_insert_pieces( /* view, pos, pieces */ );
#							ifdef notdef
	Textsw_view		view;
	register Es_index	pos;
	Es_handle		pieces;
#							endif
extern Es_status
textsw_save( /* abstract, locx, locy */ );
#							ifdef notdef
	Textsw		abstract;
	int		locx, locy;
#							endif

pkg_private Es_status
textsw_store_to_selection( /* textsw, locx, locy */ );
#							ifdef notdef
	Textsw_folio		textsw;
	int			locx, locy;
#							endif

extern Textsw_status textsw_set();
pkg_private unsigned textsw_determine_selection_type();
pkg_private void textsw_clear_secondary_selection();
pkg_private void textsw_init_selection_object();
pkg_private void textsw_update_scrollbars();
pkg_private void textsw_display_view_margins();
pkg_private void textsw_give_shelf_to_svc();
pkg_private Seln_rank seln_rank_from_textsw_info();
pkg_private void textsw_set_scroll_mark();
pkg_private void textsw_input_before();
pkg_private Key_map_handle textsw_do_filter();
pkg_private Seln_rank textsw_acquire_seln();
pkg_private void textsw_notify_replaced();
pkg_private void textsw_remove_mark_internal();


#endif

