#define MAXCARDS 4

struct graphics_ops {
	struct task_struct *owner;

	/* Board info */
	void *board_info;
	int  board_info_len;
	
	void (*save_context)(void);
	void (*restore_context)(void);
};
