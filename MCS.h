#pragma once

#include <stdbool.h>

typedef struct mcs_node {
	#ifndef ATOMIC
	struct mcs_node * volatile next;
	#else
	_Atomic(struct mcs_node *) next;
	#endif // ! ATOMIC
	VTYPE spin;
} MCS_node CALIGN;

#ifndef ATOMIC
typedef MCS_node * MCS_lock;
#else
typedef _Atomic(MCS_node *) MCS_lock;
#endif // ! ATOMIC

inline void mcs_lock( MCS_lock * lock, MCS_node * node ) {
	WO( Fence(); ); // 1
	node->next = NULL;
#ifndef MCS_OPT1										// default option
	node->spin = true;									// alternative position and remove fence below
	WO( Fence(); ); // 2
#endif // MCS_OPT1
	MCS_node * prev = Faa( lock, node );
  if ( SLOWPATH( prev == NULL ) ) return;				// no one on list ?
#ifdef MCS_OPT1
	node->spin = true;									// mark as waiting
	WO( Fence(); ); // 3
#endif // MCS_OPT1
	prev->next = node;									// add to list of waiting threads
	WO( Fence(); ); // 4
	while ( node->spin == true ) Pause();				// busy wait on my spin variable
	WO( Fence(); ); // 5
} // mcs_lock

inline void mcs_unlock( MCS_lock * lock, MCS_node * node ) {
	WO( Fence(); ); // 6
#ifdef MCS_OPT2											// original, default option
	if ( FASTPATH( node->next == NULL ) ) {				// no one waiting ?
  if ( Cas( lock, node, NULL ) ) return;				// Fence
		while ( node->next == NULL ) Pause();			// busy wait until my node is modified
	} // if
	WO( Fence(); ); // 7
	node->next->spin = false;							// stop their busy wait
#else													// Scott book Figure 4.8
	MCS_node * succ = node->next;
	if ( FASTPATH( succ == NULL ) ) {					// no one waiting ?
		// node is potentially at the tail of the MCS chain 
  if (Cas( lock, node, NULL ) ) return;					// Fence
		// Either a new thread arrived in the LD-CAS window above or fetched a false NULL
		// as the successor has yet to store into node->next.
		while ( (succ = node->next) == NULL ) Pause();	// busy wait until my node is modified
	} // if
	WO( Fence(); ); // 8
	succ->spin = false;
#endif // MCS_OPT2
} // mcs_unlock

// Local Variables: //
// tab-width: 4 //
// End: //
