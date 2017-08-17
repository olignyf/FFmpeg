//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
//						 
// toolbox_btree.h
//					
// Copyright (c) 2015 HaiVision Network Video
//
// Maintained by : Francois Oligny-Lemieux
//          Date : 22.Jan.2008
//      Modified : (see .c)
//
//  Description: 
//      Single-Threaded binairy tree
//      Elements are stored as a btree_T *
//
//  Limitations:
//      WARNING: Not multi-thread safe, protect by mutex.
//
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

#ifndef _C_TOOLBOX_GENERIC_TREE_
#define _C_TOOLBOX_GENERIC_TREE_

#ifdef __cplusplus
extern "C" {
#endif

#include "toolbox-errors.h" 
#include "toolbox-basic-types.h"  // for uint64_t
#include "toolbox-flexstring.h"

struct genericTree_S;

typedef struct treeItem_S
{
	flexString_T name;
   struct genericTree_S * tree;
	void * client;
	struct treeItem_S * parent;
	struct treeItem_S * next;
	struct treeItem_S * prev;
	struct treeItem_S * childs;
	uint64_t unique_id;
   uint64_t flags;
} treeItem_T;


typedef struct genericTree_S
{
	treeItem_T top; // top name should be a directory with no siblings, just childs
	int should_always_be_0xAFAF7878;
	int accept_duplicate_names;
	int accept_duplicate_unique_ids;
	int allow_zero_unique_ids; // usually when passing a 0 unique_id, a random one will be generated. Settings this on will keep 0 as the unique id
   int max_name_length; // 0 for unlimited, see constructor for default
	treeItem_T * current_iterator;
	uint64_t next_unique_id;
   int characterEncoding; //of type gnucTextEncoding_E
   int itemAmount;

} genericTree_T;


int genericTree_Constructor(genericTree_T * genericTree);
int genericTree_Insert(genericTree_T * genericTree, treeItem_T * insert_on_this_level, const char * name, uint64_t unique_id, void * client, treeItem_T ** new_element);

// will delete node and its child, but not free the client void *
int genericTree_Remove(genericTree_T * genericTree, treeItem_T * removeItem);

// will delete node and its child, but not free the client void *
int genericTree_Delete(genericTree_T * genericTree, treeItem_T * removeItem);

int genericTree_Destructor(genericTree_T * genericTree);



#ifdef __cplusplus
}
#endif

#endif
