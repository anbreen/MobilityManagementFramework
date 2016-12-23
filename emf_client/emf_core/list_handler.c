//-----------------------------------------------------------------------------
#include "emf.h"
//-----------------------------------------------------------------------------
 
#define show_msg 0

void append_assoc_node(struct assoc_list *assoc_node)
{
	if (show_msg)
		showmsg("\nlist_handler: append_assoc_node() function called, now inside it...");
	
	if(assoc_head == NULL)
	{
		assoc_head = assoc_node;
		assoc_node->prev = NULL;
	}
	else
	{
		assoc_tail->next = assoc_node;
		assoc_node->prev = assoc_tail;
	}
	assoc_node->next = NULL;
	assoc_tail = assoc_node;
}




void insert_assoc_node(struct assoc_list *assoc_node, struct assoc_list *after) 
{
	if (show_msg)
		showmsg("\nlist_handler: insert_assoc_node() function called, now inside it...");
	
	assoc_node->next = after->next;
	assoc_node->prev = after;

	if(after->next != NULL)
		after->next->prev = assoc_node;
	else
		assoc_tail = assoc_node;

	after->next = assoc_node;
}



void remove_assoc_node(struct assoc_list *assoc_node)
{
	if (show_msg)
		showmsg("\nlist_handler: remove_assoc_node() function called, now inside it...");
	
	if(assoc_node->prev == NULL && assoc_node->next==NULL)
	{
		assoc_head = NULL;
		assoc_tail =NULL;
		flagNodeDeleted = 0;	
	}
	else	
	{
		if(assoc_node->prev == NULL)
		{
			//printf("it was the head of list...\n");
			assoc_head = assoc_node->next;
			assoc_node->next->prev = NULL;
			nextOfDeletedNode = assoc_node->next;			// First Node Deleted
		}
		else
		{
			assoc_node->prev->next = assoc_node->next;
		}		
		if(assoc_node->next == NULL)
		{
			//printf("it was the tail of list...\n");
			assoc_node->prev->next = NULL;
			assoc_tail = assoc_node->prev;
			nextOfDeletedNode = NULL;					// Last Node Deleted
		}
		else
		{
			assoc_node->next->prev = assoc_node->prev;
			nextOfDeletedNode = assoc_node->next;		// Middle Node Deleted
		}
		flagNodeDeleted = 1;
		}
	
		free(assoc_node);
		assoc_node=0;
}

void append_emf_node(struct emf_list *emf_node)
{
	if (show_msg)
		showmsg("\nlist_handler: append_emf_node() function called, now inside it...");
	
	if(emf_head == NULL)
	{
		emf_head = emf_node;
		emf_node->prev = NULL;
	}
	else
	{
		emf_tail->next = emf_node;
		emf_node->prev = emf_tail;
	}
	
	emf_node->next = NULL;
	emf_tail = emf_node;
}

unsigned int insert_emf_node(struct emf_list *emf_node)//insert at position where list remains sorted by sequence numbers.
{
	if (show_msg)
		showmsg("\nlist_handler: insert_emf_node() function called, now inside it...");
	
	struct emf_list *before;

	for(before = emf_head; before != NULL; before = before->next)
	{
		if(emf_node->seq_no < before->seq_no)
			break;	
	}
	if(before == NULL)
	{
		append_emf_node(emf_node);
	}
	else
	if(before->prev == NULL)
	{
		emf_node->prev = before->prev;
		emf_node->next = before;
		before->prev = emf_node;
		emf_head = emf_node;
	}
	else
	{	
		emf_node->prev = before->prev;
		emf_node->next = before;
		before->prev = emf_node;
		emf_node->prev->next = emf_node;		
	}	
	unsigned int seqno=0;
	//after insert CHECK IT
	//previous seq_no + length = current sequence number
	while (emf_node != NULL)
	{
		if(emf_node->next == NULL)
		{
			seqno += emf_node->length;
			return seqno;
		}
		if(((emf_node->seq_no)+(emf_node->length))!=emf_node->next->seq_no)
		{
			seqno += emf_node->length;
			return seqno;
		}
		else
		{
			seqno += emf_node->length;	
		}	
		emf_node = emf_node->next;		
	}
	return seqno;
}

void remove_emf_node(struct emf_list *emf_node)
{
	if (show_msg)
		showmsg("\nlist_handler: remove_emf_node() function called, now inside it...");

	if(emf_node->prev == NULL && emf_node->next==NULL)
	{
		emf_head = NULL;
		emf_tail =NULL;	
	}
	else
	{
		if(emf_node->prev == NULL)
		{
			emf_head = emf_node->next;
			emf_node->next->prev = NULL;
		}
		else
		{
			emf_node->prev->next = emf_node->next;
		}		
		if(emf_node->next == NULL)
		{
			emf_node->prev->next = NULL;
			emf_tail = emf_node->prev;
		}
		else
		{
			emf_node->next->prev = emf_node->prev;
		}
	}	
	free(emf_node);
}

void append_con_send_node(struct con_slist *con_node)
{
	if(con_head == NULL)
	{
		con_head = con_node;
		con_node->prev = NULL;
	}
	else
	{
		con_tail->next = con_node;
		con_node->prev = con_tail;
	}
	con_node->next = NULL;
	con_tail = con_node;
}


void insert_con_send_node(struct con_slist *con_node, struct con_slist *after)
{
	if (show_msg)
		showmsg("\nlist_handler: insert_con_send_node() function called, now inside it...");

	con_node->next = after->next;
	con_node->prev = after;
	
	if(after->next != NULL)
		after->next->prev = con_node;
	else
		con_tail = con_node;

	after->next = con_node;
}



void remove_con_send_node(struct con_slist *con_node)
{
	if(con_node->prev == NULL && con_node->next==NULL)
	{
		con_head = NULL;
		con_tail =NULL;
	}
	else
	{
		if(con_node->prev == NULL)
		{
			con_head = con_node->next;
			con_node->next->prev = NULL;
		}
		else
		{
			con_node->prev->next = con_node->next;
		}	
		if(con_node->next == NULL)
		{
			con_node->prev->next = NULL;
			con_tail = con_node->prev;
		}
		else
		{
			con_node->next->prev = con_node->prev;
		}
	}
	free(con_node->data);
	free(con_node);
	con_node=0;
}
