#include "channel.h"

// Creates a new channel with the provided size and returns it to the caller
// A 0 size indicates an unbuffered channel, whereas a positive size indicates a buffered channel
chan_t* channel_create(size_t size)
{

    chan_t* new = (chan_t*) malloc(sizeof(chan_t));

    if (new == NULL) {
        return NULL;
    }

    if (size <= 0) 
    {
        return NULL;
    }

    

    new->buffer = buffer_create(size);
    new->close = 0;
    sem_init(&new->send, 0, (unsigned int) size);
    sem_init(&new->receive, 0, 0);

    pthread_mutex_init(&new->lock, NULL);
    pthread_mutex_init(&new->sendList, NULL);
    pthread_mutex_init(&new->receiveList, NULL);

    new->receiveLL = list_create();
    new->sendLL = list_create();



    return new;

}


// Writes data to the given channel
// This can be both a blocking call i.e., the function only returns on a successful completion of send (blocking = true), and
// a non-blocking call i.e., the function simply returns if the channel is full (blocking = false)
// In case of the blocking call when the channel is full, the function waits till the channel has space to write the new data
// Returns SUCCESS for successfully writing data to the channel,
// WOULDBLOCK if the channel is full and the data was not added to the buffer (non-blocking calls only),
// CLOSED_ERROR if the channel is closed, and
// OTHER_ERROR on encountering any other generic error of any sort
enum chan_status channel_send(chan_t* channel, void* data, bool blocking)
{

    // non blocking = try wait

    //printf("sen\n");
    if (blocking) // channel is full
    {
        //printf("start of suc1\n");
        sem_wait(&channel->send);

        pthread_mutex_lock(&channel->lock);

        if (channel->close) // check channel closed
        {
            pthread_mutex_unlock(&channel->lock);
            sem_post(&channel->send);

            return CLOSED_ERROR;
        }
        
        buffer_add(data, channel->buffer);
        pthread_mutex_unlock(&channel->lock);

        sem_post(&channel->receive);
        
        // here you want to lock which ever list you are editing, and then do list_foreach inside the lock
        pthread_mutex_lock(&channel->receiveList);
        // for each on recv linkedlist
        //printf("2\n");
        if (list_count(channel->receiveLL) >0) {
            //printf("4\n");
            list_foreach(channel->receiveLL, (void *) sem_post);
            //printf("3\n");
        }

        pthread_mutex_unlock(&channel->receiveList);

        //printf("end of suc1\n");

        return SUCCESS;
        
        
    }
    else
    {
        //printf("start of suc2\n");
        if (sem_trywait(&channel->send) != 0) {

			pthread_mutex_lock(&channel->lock);
            if (channel->close) // check channel closed
            {
				pthread_mutex_unlock(&channel->lock);
                return CLOSED_ERROR;
            }
            pthread_mutex_unlock(&channel->lock);
            
            return WOULDBLOCK;
        }
        //printf("mid of suc2\n");
        pthread_mutex_lock(&channel->lock);

        if (channel->close) // check channel closed
        {
            pthread_mutex_unlock(&channel->lock);
            sem_post(&channel->send);

            return CLOSED_ERROR;
        }

        buffer_add(data, channel->buffer);
        pthread_mutex_unlock(&channel->lock);

        sem_post(&channel->receive);

        // recv list lock
        pthread_mutex_lock(&channel->receiveList);
        // for each on recv linkedlist
        if (list_count(channel->receiveLL) >0) {
            list_foreach(channel->receiveLL, (void *) sem_post);
        }

        pthread_mutex_unlock(&channel->receiveList);

        //printf("end of suc2\n");
        return SUCCESS;
        
    }

    return OTHER_ERROR;

}

// Reads data from the given channel and stores it in the function’s input parameter, data (Note that it is a double pointer).
// This can be both a blocking call i.e., the function only returns on a successful completion of receive (blocking = true), and
// a non-blocking call i.e., the function simply returns if the channel is empty (blocking = false)
// In case of the blocking call when the channel is empty, the function waits till the channel has some data to read
// Returns SUCCESS for successful retrieval of data,
// WOULDBLOCK if the channel is empty and nothing was stored in data (non-blocking calls only),
// CLOSED_ERROR if the channel is closed, and
// OTHER_ERROR on encountering any other generic error of any sort
enum chan_status channel_receive(chan_t* channel, void** data, bool blocking)
{

    //printf("rec\n");
    if (blocking) // channel is full
    {
        sem_wait(&channel->receive);

        pthread_mutex_lock(&channel->lock);

        if (channel->close) // check channel closed
        {
            
            pthread_mutex_unlock(&channel->lock);
            
            sem_post(&channel->receive);
            
            //printf("11\n");
            return CLOSED_ERROR;
        }

        *data = buffer_remove(channel->buffer);
        pthread_mutex_unlock(&channel->lock);

        sem_post(&channel->send);

        //printf("22\n");

        // here you want to lock which ever list you are editing, and then do list_foreach inside the lock


        // send list lock
        pthread_mutex_lock(&channel->sendList);
        // for each on recv linkedlist
        if(list_count(channel->sendLL) > 0) {
            list_foreach(channel->sendLL, (void *) sem_post);
        }
        //printf("33\n");
        
        pthread_mutex_unlock(&channel->sendList);

        return SUCCESS;    

        
    }
    else
    {
        if (sem_trywait(&channel->receive) != 0) 
        {
			pthread_mutex_lock(&channel->lock);
            if (channel->close) // check channel closed
            {
				pthread_mutex_unlock(&channel->lock);
                return CLOSED_ERROR;
            }
            pthread_mutex_unlock(&channel->lock);
            
            return WOULDBLOCK;
        }

        pthread_mutex_lock(&channel->lock);

        if (channel->close) // check channel closed
        {
            
            pthread_mutex_unlock(&channel->lock);
            
            sem_post(&channel->receive);

            return CLOSED_ERROR;
        }

        *data = buffer_remove(channel->buffer);
        pthread_mutex_unlock(&channel->lock);

        sem_post(&channel->send);

        // here you want to lock which ever list you are editing, and then do list_foreach inside the lock
               

        // send list lock
        pthread_mutex_lock(&channel->sendList);
        // for each on recv linkedlist
        if(list_count(channel->sendLL) > 0) {
            list_foreach(channel->sendLL, (void *) sem_post);
        }
        
        pthread_mutex_unlock(&channel->sendList);

        return SUCCESS;    
        
    }
    
    return OTHER_ERROR;

}


// Closes the channel and informs all the blocking send/receive/select calls to return with CLOSED_ERROR
// Once the channel is closed, send/receive/select operations will cease to function and just return CLOSED_ERROR
// Returns SUCCESS if close is successful,
// CLOSED_ERROR if the channel is already closed, and
// OTHER_ERROR in any other error case
enum chan_status channel_close(chan_t* channel)
{

    //printf("clo\n");
    // do everything in lock if it fails
    pthread_mutex_lock(&channel->lock);
    if (channel->close == 1) // channel is closed
    {
        pthread_mutex_unlock(&channel->lock);
        return CLOSED_ERROR;
    }
    else if (channel->close == 0) // channel is open
    {
        channel->close = 1;
        pthread_mutex_unlock(&channel->lock);
        sem_post(&channel->send);
        sem_post(&channel->receive);

        pthread_mutex_lock(&channel->receiveList);

        if (list_count(channel->receiveLL) > 0) {
            list_foreach(channel->receiveLL, (void *) sem_post);
        }

        pthread_mutex_unlock(&channel->receiveList);


        pthread_mutex_lock(&channel->sendList);
        if (list_count(channel->sendLL) >0) {
            list_foreach(channel->sendLL, (void *) sem_post);
        }
        pthread_mutex_unlock(&channel->sendList);



        return SUCCESS;
    }
    
    pthread_mutex_unlock(&channel->lock);

    return OTHER_ERROR;

}


// Frees all the memory allocated to the channel
// The caller is responsible for calling channel_close and waiting for all threads to finish their tasks before calling channel_destroy
// Returns SUCCESS if destroy is successful,
// DESTROY_ERROR if channel_destroy is called on an open channel, and
// OTHER_ERROR in any other error case
enum chan_status channel_destroy(chan_t* channel)
{
    if (channel->close)
    {
        sem_destroy(&channel->send);
        sem_destroy(&channel->receive);
        pthread_mutex_destroy(&channel->lock);

        // send mutex
        // rec mutex
        // linkdestory(send) & recv

        pthread_mutex_destroy(&channel->sendList);
        pthread_mutex_destroy(&channel->receiveList);
        list_destroy(channel->sendLL);
        list_destroy(channel->receiveLL);

        if (channel->buffer)    //buffer not empty
        {
            buffer_free(channel->buffer);   
        }
            
        free(channel);

        return SUCCESS;
    }
    else if (channel->close == 0)
    {
        return DESTROY_ERROR;
    }

    return OTHER_ERROR;

}


// Takes an array of channels, channel_list, of type select_t and the array length, channel_count, as inputs
// This API iterates over the provided list and finds the set of possible channels which can be used to invoke the required operation (send or receive) specified in select_t
// If multiple options are available, it selects the first option and performs its corresponding action
// If no channel is available, the call is blocked and waits till it finds a channel which supports its required operation
// Once an operation has been successfully performed, select should set selected_index to the index of the channel that performed the operation and then return SUCCESS
// In the event that a channel is closed or encounters any error, the error should be propagated and returned through select
// Additionally, selected_index is set to the index of the channel that generated the error
enum chan_status channel_select(size_t channel_count, select_t* channel_list, size_t* selected_index)
{
    //printf("sel\n");
    enum chan_status check = OTHER_ERROR;

    // add to the list of send or receive

    
    // for (size_t i = 0 ; i < channel_count ; i++)
    // {
    
    //     if (channel_list[i].is_send) //checks to see if the channel is a send or recev
    //     {

    //         check = channel_send(channel_list[i].channel, channel_list[i].data, false);

    //     }
        
    //     else
    //     {
    //         check = channel_receive(channel_list[i].channel, &channel_list[i].data, false);
    //     }


    //     if (check != WOULDBLOCK)
    //     {
    //         selected_index = (size_t *) i;

    //         return check;

    //     }

    // }
    
    
   
    sem_t local;

    sem_init(&local, 0,  0);

    // adding semphaore to one list or the other
    for (size_t i = 0 ; i < channel_count ; i++)
    {
        if (channel_list[i].is_send) //checks to see if the channel is a send or recev
        {
            
            pthread_mutex_lock(&(channel_list[i].channel)->sendList);
            list_node_t* found = list_find(channel_list[i].channel->sendLL, &local);
            

            if (!found)
            {
                list_insert(channel_list[i].channel->sendLL, &local);
            }

            pthread_mutex_unlock(&(channel_list[i].channel)->sendList);
            

        }
        
        else
        {
            
            pthread_mutex_lock(&channel_list[i].channel->receiveList);
            list_node_t* found = list_find(channel_list[i].channel->receiveLL, &local);
           
            if (!found)
            {
                list_insert(channel_list[i].channel->receiveLL, &local);
            }
            pthread_mutex_unlock(&channel_list[i].channel->receiveList);
            
        }

    } 


    while (true)
    {
        //size_t new = 0;
        //printf("5\n");
        for (size_t i = 0 ; i < channel_count ; i++)
        {
            if (channel_list[i].is_send) //checks to see if the channel is a send or recev
            {
                //new = (size_t) i;
                check = channel_send(channel_list[i].channel, channel_list[i].data, false);
            }
            
            else
            {
                //new = (size_t) i;
                check = channel_receive(channel_list[i].channel, &channel_list[i].data, false);
            }

            //new = (size_t) i;

            //printf("4\n");
            if (check != WOULDBLOCK)
            {
                //printf("4\n");
                *selected_index = i;

                for(size_t i = 0; i<channel_count;i++) 
                {
                    if (channel_list[i].is_send) //checks to see if the channel is a send or recev
                    {

                        pthread_mutex_lock(&(channel_list[i].channel)->sendList);
                        list_node_t* found = list_find(channel_list[i].channel->sendLL, &local);
                        

                        if (found)
                        {
                            list_remove(channel_list[i].channel->sendLL, found);
                        }

                        pthread_mutex_unlock(&(channel_list[i].channel)->sendList);


                    }
                    
                    else
                    {
                        
                        pthread_mutex_lock(&(channel_list[i].channel)->receiveList);
                        list_node_t* found = list_find(channel_list[i].channel->receiveLL, &local);

                        if (found)
                        {
                            list_remove(channel_list[i].channel->receiveLL, found);
                        }
                        pthread_mutex_unlock(&(channel_list[i].channel)->receiveList);

                    }
                }

                //printf("5\n");
                sem_destroy(&local);
                //printf("6\n");

                return check;
            }

        }

        sem_wait(&local);
        

    }

    


    return check;


    // create a local semaphore
    // add the semaphore to linked list of send or receive
    // -------
    // while loop
    // for each in list perform send or receive
    // if none work then sem wait -> will go back into while loop until an action is observed
    // -------
    // remove the local semaphore
    // return 

    // does send non-blocking
    // does receive non-blocking
}
