#ifndef TOPMAN_H
#define TOPMAN_H

/** @file topmanager.h 
 *
 * @brief Topology Manager interface.
 *
 * This is the Topology Manager interface. See @link topology_test.c
 * topology_test.c @endlink for an usage example
 *
 */

/** @example topology_test.c
 * 
 * A test program showing how to use the Topology Manager API.
 *
 */

/**
   @brief Mantains the context of a topology manager instance
 */
struct top_context;

/**
  @brief Get the peer's neighbourhood.

  This function returns the current neighbourhood (i.e., the set of
  known peers) of a peer, and its size. Note that the current neighbourhood
  size can be different from the requested one, because of how the overlay
  management protocols work. 

  @param tc the pointer to the current topology manager instance context
  @param n pointer to an integer where the neighbourhood size is returned.
           Is set to -1 in case of error, or 0 if the neighbourhood is empty.
  @return a pointer to an array of nodeID describing the neighbourhood. NULL
          in case of error, or if the neighbourhood is empty.
*/
const struct nodeID **topGetNeighbourhood(struct top_context *tc, int *n);

/**
  @brief Get the peer's metadata.

  Each peer in the neighbourhood can have some opaque metadata attached to
  it, and such metadata is gossiped together with the nodeIDs.
  This function returns the metadata currently associated to the
  neighbours of a peer, and the size of each object composing the metadata
  (metadata have the same structure for all the peers, so such size is
  constant). The neighbourhood size can be known by calling
  topGetNeighbourhood().

  @param tc the pointer to the current topology manager instance context
  @param metadata_size pointer to an integer where the size of the metadata
         associated to each peer is returned.
         Is set to -1 in case of error, or 0 if the neighbourhood is empty.
  @return a pointer to an array of metadata (ordered as the peers returned
          by topGetNeighbourhood()).
          NULL in case of error, or if the neighbourhood is empty.
*/
const void *topGetMetadata(struct top_context *tc, int *metadata_size);

/**
  @brief Increase the neighbourhood size.

  This function can be used to increase the number of neighbours (that is,
       the degree of the overlay graph) by a specified amount.
  @param tc the pointer to the current topology manager instance context
  @param n number of peers by which the neighbourhood size must be incremented.
  @return the new neighbourhood size in case of success; -1 in case of error.
*/
int topGrowNeighbourhood(struct top_context *tc, int n);

/**
  @brief Decrease the neighbourhood size.

  This function can be used to reduce the number of neighbours (that is,
       the degree of the overlay graph) by a specified amount.
  @param tc the pointer to the current topology manager instance context
  @param n number of peers by which the neighbourhood size must be decreased.
  @return the new neighbourhood size in case of success; -1 in case of error.
*/
int topShrinkNeighbourhood(struct top_context *tc, int n);

/**
  @brief Remove a peer from the neighbourhood.

  This function can be used to remove a specified neighbour from the
  neighbourhood. Note that the requested neighbourhood size is not
  modified, so the peer will be soon replaced by a different one.
  @param tc the pointer to the current topology manager instance context
  @param neighbour the id of the peer to be removed from the
         neighbourhood.
  @return 0 in case of success; -1 in case of error.
*/
int topRemoveNeighbour(struct top_context *tc, struct nodeID *neighbour);

/**
  @brief Change the metadata.

  This function can be used to modify/update the metadata for a
  peer. Because of security concerns, only the metadata for the
  local peer can be modified.
  @param tc the pointer to the current topology manager instance context
  @param metadata pointer to the new metadata associated to the peer (will be
         gossiped).
  @param metadata_size size of the metadata associated to the peer (must
         be the same as for the other peers).
  @return 0 in case of success; -1 in case of error.
*/
int topChangeMetadata(struct top_context *tc, void *metadata, int metadata_size);

/**
  @brief Initialise the Topology Manager.

  @param myID the ID of this peer.
  @param metadata pointer to the metadata associated to this peer (will be
         gossiped).
  @param metadata_size size of the metadata associated to this peer.
  @param context points to memory location where will be stored the pointer to the
         current instance topology manager context
  @return 0 in case of success; -1 in case of error.
*/
int topInit(struct nodeID *myID, void *metadata, int metadata_size, const char *config, struct top_context **context);

/**
  @brief Insert a peer in the neighbourhood.

  This function can be used to add a specified neighbour to the
  neighbourhood. It is useful in the bootstrap phase.
  @param tc the pointer to the current topology manager instance context
  @param neighbour the id of the peer to be added to the
         neighbourhood.
  @param metadata pointer to the metadata associated to the new peer (will be
         gossiped).
  @param metadata_size size of the metadata associated to the new peer (must
         be the same as for the other peers).
  @return 0 in case of success; -1 in case of error.
*/
int topAddNeighbour(struct top_context *tc, struct nodeID *neighbour, void *metadata, int metadata_size);

/**
  @brief Pass a received packet to the Topology Manager.

  This function passes to the Topology Manager a packet that has
  been received from the network. The Topology Manager will parse
  such packet and run the protocol, adding or removing peers to the
  neighbourhood, and sending overlay management messages to other peers.
  @param tc the pointer to the current topology manager instance context
  @param buff a memory buffer containing the received message.
  @param len the size of such a memory buffer.
  @return 0 in case of success; -1 in case of error.
*/
int topParseData(struct top_context *tc, const uint8_t *buff, int len);

#endif /* TOPMAN_H */
