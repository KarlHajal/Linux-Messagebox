#include <linux/kernel.h>
#include <lab3/messagebox.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/random.h>
#include <linux/string.h>

struct mb_list{
	struct list_head list;
	struct messagebox msgbox;
};

LIST_HEAD(all_mbs);

asmlinkage long sys_create_messagebox(struct messagebox *mb){
	struct messagebox *mbx;
	mbx = kmalloc(sizeof(struct messagebox), GFP_KERNEL);
	if(access_ok(VERIFY_WRITE, mb, sizeof(struct messagebox)) == 0)
		return -1;	
	copy_from_user(mbx, mb, sizeof(struct messagebox));
	
	int rand_num;
	get_random_bytes(&rand_num, sizeof(rand_num));
	rand_num &= 0x7FFFFFFF;
	mbx->mb_id = rand_num;
    	mbx->max_subscribers = 10;
    	mbx->number_of_subscribers = 0;

	mbx->messages = kmalloc(20*sizeof(mb), GFP_KERNEL);

    	mbx->max_messages = 10;
    	mbx->number_of_messages = 0;
	struct mb_list *mbs;
	mbs = kmalloc(sizeof(struct mb_list), GFP_KERNEL);
	mbs->msgbox = *mbx;
	list_add_tail(&(mbs->list), &all_mbs);
	return mbx->mb_id;
}

asmlinkage long sys_list_all_messageboxes(struct messagebox *mbs, int maxcount){
	int count = 0;
	struct list_head *pos;
	struct mb_list *tmp;
	list_for_each(pos, &all_mbs){
		if(count >= maxcount)
			return count;
		tmp = list_entry(pos, struct mb_list, list);
		copy_to_user(mbs + count*sizeof(struct messagebox), &(tmp->msgbox), sizeof(struct messagebox));
		count++;
	}	
	return count;
}

asmlinkage long sys_list_messageboxes_by_topic(char *topic, int topic_length, struct messagebox *mbs, int maxcount){
	int count = 0;
	struct list_head *pos;
	struct mb_list *tmp;
	char *temp_topic;
	temp_topic = kmalloc(sizeof(char)*(topic_length+1), GFP_KERNEL);
	list_for_each(pos, &all_mbs){
		if(count >= maxcount)
			return count;
		tmp = list_entry(pos, struct mb_list, list);
		copy_from_user(temp_topic, topic, sizeof(char)*topic_length);
        temp_topic[topic_length] = '\0';
		printk("%s || %s \n",(&(tmp->msgbox))->topic , temp_topic); //TESTING
        if(strcmp((&(tmp->msgbox))->topic, temp_topic) == 0){
			copy_to_user(mbs + count*sizeof(struct messagebox), &(tmp->msgbox), sizeof(struct messagebox));
			count++;
		}
	}	
	return count;
}

asmlinkage long sys_subscribe_to_messagebox(int mb_id, int pid){
    struct list_head *pos;
    struct mb_list *tmp;
    list_for_each(pos, &all_mbs){
        tmp = list_entry(pos, struct mb_list, list);
        if(tmp->msgbox.mb_id == mb_id && tmp->msgbox.number_of_subscribers < tmp->msgbox.max_subscribers){
            tmp->msgbox.subscribers_id[tmp->msgbox.number_of_subscribers] = pid;
            tmp->msgbox.number_of_subscribers++;
            printk("User %i has subscribed to messagebox %i, number of subscribers: %i\n", pid, tmp->msgbox.mb_id, tmp->msgbox.number_of_subscribers); 
            return 1;
        }
    }
    return -1;
}


asmlinkage long sys_get_number_of_subscribers(int mb_id){
    struct list_head *pos;
    struct mb_list *tmp;
    list_for_each(pos, &all_mbs){
        tmp = list_entry(pos, struct mb_list, list);
        if(tmp->msgbox.mb_id == mb_id)
            return tmp->msgbox.number_of_subscribers;
    }
    return -1;
}


asmlinkage long sys_list_subscribers(int mb_id, int *pids, int maxcount){
    int count = 0;
    struct list_head *pos;
    struct mb_list *tmp;
    list_for_each(pos, &all_mbs){
        tmp = list_entry(pos, struct mb_list, list);
        if(tmp->msgbox.mb_id == mb_id){
            printk("Number of subscribers %i || Max: %i\n", tmp->msgbox.number_of_subscribers, maxcount); //TESTING
            if(maxcount < tmp->msgbox.number_of_subscribers)
                count = maxcount;
            else
                count = tmp->msgbox.number_of_subscribers;
            copy_to_user(pids, tmp->msgbox.subscribers_id, count*sizeof(int));
            return count;
        }
    }
    return -1;
}


asmlinkage long sys_send_message(int mb_id, char *message, int length, int pid){
    struct list_head *pos;
    struct mb_list *tmp;
    int rand;
    list_for_each(pos, &all_mbs){
        tmp = list_entry(pos, struct mb_list, list);
        if(tmp->msgbox.mb_id == mb_id){
            if(tmp->msgbox.number_of_messages >= tmp->msgbox.max_messages)
                return -1;
            struct message *msg;
            msg = kmalloc(sizeof(struct message), GFP_KERNEL);
            if(pid == 0)
                msg->type = 1; //BROADCAST
            else{
                msg->type = 2; //UNICAST
                msg->pid = pid;
            }
            get_random_bytes(&rand, sizeof(rand));
            rand &= 0x7FFFFFFF;
            msg->m_id = rand;
            msg->length = length;
            msg->message = kmalloc(sizeof(char)*(length+1), GFP_KERNEL);
            copy_from_user(msg->message, message, length*sizeof(char));
            msg->message[length] = '\0';
            tmp->msgbox.messages[tmp->msgbox.number_of_messages] = msg;
            printk("Message %s sent to messagebox %i (Message id %i)\n", msg->message, mb_id, msg->m_id); //TESTING
            tmp->msgbox.number_of_messages++;
            return 0;
        }
    }
    return -1;
}


asmlinkage long sys_get_list_of_messages(int mb_id, int *mids, int *length, int maxcount){
    struct list_head *pos;
    struct mb_list *tmp;
    list_for_each(pos, &all_mbs){
        tmp = list_entry(pos, struct mb_list, list);
        if(tmp->msgbox.mb_id == mb_id){
            int count;
            for(count = 0; count < tmp->msgbox.number_of_messages && count < maxcount; count++){
                copy_to_user(mids + count*sizeof(int), &(tmp->msgbox.messages[count]->m_id), sizeof(int));
                copy_to_user(length + count*sizeof(int), &(tmp->msgbox.messages[count]->length), sizeof(int));
            }
            return count;
        }
    }
    return -1;
}


asmlinkage long sys_get_message(int mb_id, int mid, char *message, int maxlength, int user_id){
    struct list_head *pos;
    struct mb_list *tmp;
    list_for_each(pos, &all_mbs){
        tmp = list_entry(pos, struct mb_list, list);
        if(tmp->msgbox.mb_id == mb_id){
            int i;
            for(i=0; i<tmp->msgbox.number_of_messages; i++){
                if(tmp->msgbox.messages[i]->m_id == mid && (tmp->msgbox.messages[i]->type == 1 || (tmp->msgbox.messages[i]->pid == user_id))){
                    if(tmp->msgbox.messages[i]->length < maxlength)
                        copy_to_user(message, tmp->msgbox.messages[i]->message, sizeof(char)*tmp->msgbox.messages[i]->length);
                    else
                        copy_to_user(message, tmp->msgbox.messages[i]->message, sizeof(char)*maxlength);
                    return 1;
                }
            }
            return -1;
        }
    }
    return -1;
}

