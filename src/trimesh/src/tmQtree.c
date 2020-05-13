#include "trimesh/tmTypedefs.h"
#include "trimesh/tmMesh.h"
#include "trimesh/tmEdge.h"
#include "trimesh/tmNode.h"
#include "trimesh/tmTri.h"
#include "trimesh/tmQtree.h"

/**********************************************************
* Function: tmQtree_split()
*----------------------------------------------------------
* Split a tmQtree into four smaller tmQtrees
*----------------------------------------------------------
* @param qtree: tmQtree structure
**********************************************************/
static void tmQtree_split(tmQtree *qtree);

/**********************************************************
* Function: tmQtree_getObjNo()
*----------------------------------------------------------
* Merge a tmQtree from its four smaller tmQtrees
*----------------------------------------------------------
* @param qtree: tmQtree structure
**********************************************************/
static tmBool tmQtree_merge(tmQtree *qtree);


/**********************************************************
* Function: tmQtree_split()
*----------------------------------------------------------
* Split a tmQtree into four smaller tmQtrees
*----------------------------------------------------------
* @param qtree: tmQtree structure
**********************************************************/
static void tmQtree_split(tmQtree *qtree)
{
  /*-------------------------------------------------------
  | Create new children structures
  -------------------------------------------------------*/
  tmDouble xy_min[2] = { 0.0, 0.0 };
  tmDouble xy_max[2] = { 0.0, 0.0 };

  qtree->child_NE = tmQtree_create(qtree->mesh, 
                                   qtree->obj_type);
  xy_min[0] = qtree->xy[0];
  xy_min[1] = qtree->xy[1];
  xy_max[0] = qtree->xy_max[0];
  xy_max[1] = qtree->xy_max[1];

  tmQtree_init(qtree->child_NE, qtree, 
               qtree->layer + 1,
               xy_min, xy_max);


  qtree->child_NW = tmQtree_create(qtree->mesh,
                                   qtree->obj_type);
  xy_min[0] = qtree->xy_min[0];
  xy_min[1] = qtree->xy[1];
  xy_max[0] = qtree->xy[0];
  xy_max[1] = qtree->xy_max[1];
  tmQtree_init(qtree->child_NW, qtree, 
               qtree->layer + 1,
               xy_min, xy_max);


  qtree->child_SW = tmQtree_create(qtree->mesh,
                                   qtree->obj_type);
  xy_min[0] = qtree->xy_min[0];
  xy_min[1] = qtree->xy_min[1];
  xy_max[0] = qtree->xy[0];
  xy_max[1] = qtree->xy[1];
  tmQtree_init(qtree->child_SW, qtree, 
               qtree->layer + 1,
               xy_min, xy_max);


  qtree->child_SE = tmQtree_create(qtree->mesh,
                                   qtree->obj_type);
  xy_min[0] = qtree->xy[0];
  xy_min[1] = qtree->xy_min[1];
  xy_max[0] = qtree->xy_max[0];
  xy_max[1] = qtree->xy[1];
  tmQtree_init(qtree->child_SE, qtree, 
               qtree->layer + 1,
               xy_min, xy_max);

  /*-------------------------------------------------------
  | Distribute objects to children
  -------------------------------------------------------*/
  ListNode *cur, *nxt;
  tmDouble *cur_xy;

  for (cur = qtree->obj->first; 
       cur != NULL; cur = cur->next)
  {
    if ( qtree->obj_type == TM_NODE)
      cur_xy = ((tmNode*)cur->value)->xy;
    else if ( qtree->obj_type == TM_EDGE)
      cur_xy = ((tmEdge*)cur->value)->xy;
    else if ( qtree->obj_type == TM_TRI)
      cur_xy = ((tmTri*)cur->value)->xy;
    else
      log_err("Wrong type provied for tmQtree_split()");

    if ( cur_xy[0] >= qtree->xy[0] )
    {
      if ( cur_xy[1] >= qtree->xy[1] )
        tmQtree_addObj(qtree->child_NE, cur->value);
      else
        tmQtree_addObj(qtree->child_SE, cur->value);
    }
    else
    {
      if ( cur_xy[1] >= qtree->xy[1] )
        tmQtree_addObj(qtree->child_NW, cur->value);
      else
        tmQtree_addObj(qtree->child_SW, cur->value);
    }
  }

  /*-------------------------------------------------------
  | Clear own object list
  -------------------------------------------------------*/
  cur = qtree->obj->first;
  nxt = cur;
  while (nxt != NULL)
  {
    nxt = cur->next;
    tmQtree_remObj(qtree, cur->value);
    cur = nxt;
  }

  /*-------------------------------------------------------
  | Qtree is now splitted
  -------------------------------------------------------*/
  qtree->is_splitted = TRUE;

} /* tmQtree_split() */

/**********************************************************
* Function: tmQtree_getObjNo()
*----------------------------------------------------------
* Merge a tmQtree from its four smaller tmQtrees
*----------------------------------------------------------
* @param qtree: tmQtree structure
**********************************************************/
static tmBool tmQtree_merge(tmQtree *qtree)
{
  ListNode *cur;
  
  /*-------------------------------------------------------
  | Get sure, that children are not splitted too
  -------------------------------------------------------*/
  check(qtree->child_NE->is_splitted == FALSE,
      "Can not merge qtree -> child_NE is still splitted.");
  check(qtree->child_NW->is_splitted == FALSE,
      "Can not merge qtree -> child_NW is still splitted.");
  check(qtree->child_SW->is_splitted == FALSE,
      "Can not merge qtree -> child_SW is still splitted.");
  check(qtree->child_SE->is_splitted == FALSE,
      "Can not merge qtree -> child_SE is still splitted.");

  /*-------------------------------------------------------
  | Set qtree to not splitted
  -------------------------------------------------------*/
  qtree->is_splitted = FALSE;

  /*-------------------------------------------------------
  | Get objects from all children
  -------------------------------------------------------*/
  /*                    NORTH EAST                       */
  for(cur = qtree->child_NE->obj->first;
      cur != NULL; cur = cur->next)
    tmQtree_addObj(qtree, cur->value);
  /*                    NORTH WEST                       */
  for(cur = qtree->child_NW->obj->first;
      cur != NULL; cur = cur->next)
    tmQtree_addObj(qtree, cur->value);
  /*                    SOUTH WEST                       */
  for(cur = qtree->child_SW->obj->first;
      cur != NULL; cur = cur->next)
    tmQtree_addObj(qtree, cur->value);
  /*                    SOUTH EAST                       */
  for(cur = qtree->child_SE->obj->first;
      cur != NULL; cur = cur->next)
    tmQtree_addObj(qtree, cur->value);

  /*-------------------------------------------------------
  | Destroy all children
  -------------------------------------------------------*/
  if (qtree->child_NE != NULL)
  {
    tmQtree_destroy(qtree->child_NE);
    qtree->child_NE = NULL;
  }
  if (qtree->child_NW != NULL)
  {
    tmQtree_destroy(qtree->child_NW);
    qtree->child_NW = NULL;
  }
  if (qtree->child_SW != NULL)
  {
    tmQtree_destroy(qtree->child_SW);
    qtree->child_SW = NULL;
  }
  if (qtree->child_SE != NULL)
  {
    tmQtree_destroy(qtree->child_SE);
    qtree->child_SE = NULL;
  }

  /*-------------------------------------------------------
  | some final checks
  -------------------------------------------------------*/
  check(qtree->n_obj <= qtree->max_obj,
      "Something went wrong while merging qtree.");
  check(qtree->is_splitted == FALSE,
      "Something went wrong while merging qtree.");

  return TRUE;

error:
  return FALSE;

} /* tmQtree_merge() */




/**********************************************************
* Function: tmQtree_create()
*----------------------------------------------------------
* Create a new tmQtree structure and return a pointer
* to it.
*----------------------------------------------------------
* @param mesh: parent mesh of the new Qtree
* @param obj_type: object type of this Qtree
*
* @return: Pointer to a new tmQtree structure
**********************************************************/
tmQtree *tmQtree_create(tmMesh *mesh, int obj_type)
{
  tmQtree *qtree = (tmQtree*) calloc( 1, sizeof(tmQtree) );
  check_mem(qtree);

  qtree->mesh   = mesh;

  /*-------------------------------------------------------
  | Qtree parent and children
  -------------------------------------------------------*/
  qtree->parent   = NULL;
  qtree->child_NE = NULL;
  qtree->child_NW = NULL;
  qtree->child_SW = NULL;
  qtree->child_SE = NULL;

  /*-------------------------------------------------------
  | Qtree properties
  -------------------------------------------------------*/
  qtree->is_splitted = FALSE;
  qtree->layer       = 0;
  qtree->max_obj     = 0;

  /*-------------------------------------------------------
  | Qtree geometric properties
  -------------------------------------------------------*/
  qtree->xy_min[0] = 0.0;
  qtree->xy_min[1] = 0.0;

  qtree->xy_max[0] = 0.0;
  qtree->xy_max[1] = 0.0;

  qtree->xy[0]     = 0.0;
  qtree->xy[1]     = 0.0;

  qtree->dxy[0]    = 0.0;
  qtree->dxy[1]    = 0.0;

  /*-------------------------------------------------------
  | Qtree objects
  -------------------------------------------------------*/
  qtree->n_obj    = 0;
  qtree->obj      = List_create();
  qtree->obj_type = obj_type;

  return qtree;
error:
  return NULL;

} /* tmQtree_create() */

/**********************************************************
* Function: tmQtree_destroy()
*----------------------------------------------------------
* Destroys a tmQtree structure and frees all its memory.
*----------------------------------------------------------
* @param *mesh: pointer to a tmQtree to destroy
**********************************************************/
void tmQtree_destroy(tmQtree *qtree)
{
  /*-------------------------------------------------------
  | Free all list structures
  -------------------------------------------------------*/
  List_destroy(qtree->obj);

  /*-------------------------------------------------------
  | Free all children structures
  -------------------------------------------------------*/
  if (qtree->child_NE != NULL)
    tmQtree_destroy(qtree->child_NE);
  if (qtree->child_NW != NULL)
    tmQtree_destroy(qtree->child_NW);
  if (qtree->child_SW != NULL)
    tmQtree_destroy(qtree->child_SW);
  if (qtree->child_SE != NULL)
    tmQtree_destroy(qtree->child_SE);

  /*-------------------------------------------------------
  | Finally free edge structure memory
  -------------------------------------------------------*/
  free(qtree);

} /* tmQtree_destroy() */

/**********************************************************
* Function: tmQtree_init()
*----------------------------------------------------------
* Initialize a tmQtree structure 
*----------------------------------------------------------
* @param qtree: tmQtree structure to initialize
*
**********************************************************/
void tmQtree_init(tmQtree *qtree, 
                  tmQtree *parent,    tmIndex  layer, 
                  tmDouble xy_min[2], tmDouble xy_max[2])
{
  qtree->layer  = layer;
  qtree->parent = parent;

  qtree->max_obj = TM_QTREE_MAX_OBJ;

  qtree->xy_min[0] = xy_min[0];
  qtree->xy_min[1] = xy_min[1];
  qtree->xy_max[0] = xy_max[0];
  qtree->xy_max[1] = xy_max[1];

  qtree->dxy[0] = xy_max[0] - xy_min[0];
  qtree->dxy[1] = xy_max[1] - xy_min[1];

  qtree->xy[0]  = 0.5 * (xy_max[0] + xy_min[0]);
  qtree->xy[1]  = 0.5 * (xy_max[1] + xy_min[1]);

} /* tmQtree_init() */


/**********************************************************
* Function: tmQtree_addObj()
*----------------------------------------------------------
* Try to add an object to the qtree 
*----------------------------------------------------------
* @param qtree: tmQtree structure
* @param obj:   object to add to the qtree
**********************************************************/
tmBool tmQtree_addObj(tmQtree *qtree, void *obj)
{
  tmBool    in_bbox;
  tmDouble *xy;

  /*-------------------------------------------------------
  | Check if object is contained within bbox of qtree
  -------------------------------------------------------*/
  if ( qtree->obj_type == TM_NODE)
    xy = ((tmNode*)obj)->xy;
  else if ( qtree->obj_type == TM_EDGE)
    xy = ((tmEdge*)obj)->xy;
  else if ( qtree->obj_type == TM_TRI)
    xy = ((tmTri*)obj)->xy;
  else
    log_err("Wrong type provied for tmQtree_addObj()");

  in_bbox = IN_ON_BBOX(xy, qtree->xy_min, qtree->xy_max);

  if (in_bbox == FALSE)
    return FALSE;
  
  /*-------------------------------------------------------
  | Distribute obj to children if qtree is already splitted
  -------------------------------------------------------*/
  if (qtree->is_splitted == TRUE)
  {
    tmBool added;
    added = tmQtree_addObj(qtree->child_NE, obj);

    if ( added == FALSE )
      added = tmQtree_addObj(qtree->child_NW, obj);
    if ( added == FALSE )
      added = tmQtree_addObj(qtree->child_SW, obj);
    if ( added == FALSE )
      added = tmQtree_addObj(qtree->child_SE, obj);
    if ( added == FALSE )
      log_err("Failed to add object to tmQtree.");

    return added;
  }
  /*-------------------------------------------------------
  | Add obj to this qtree
  -------------------------------------------------------*/
  else
  {
    List_push(qtree->obj, obj);
    qtree->n_obj += 1;

    if (qtree->n_obj > qtree->max_obj)
      tmQtree_split(qtree);

    return TRUE;
  }

} /* tmQtree_addObj() */


/**********************************************************
* Function: tmQtree_remObj()
*----------------------------------------------------------
* Try to remove an object from the qtree 
*----------------------------------------------------------
* @param qtree: tmQtree structure
* @param obj:   object to remove from the qtree
**********************************************************/
tmBool tmQtree_remObj(tmQtree *qtree, void *obj)
{
  tmBool in_bbox;
  tmDouble *xy;

  /*-------------------------------------------------------
  | Check if object is contained within bbox of qtree
  -------------------------------------------------------*/
  if ( qtree->obj_type == TM_NODE)
    xy = ((tmNode*)obj)->xy;
  else if ( qtree->obj_type == TM_EDGE)
    xy = ((tmEdge*)obj)->xy;
  else if ( qtree->obj_type == TM_TRI)
    xy = ((tmTri*)obj)->xy;
  else
    log_err("Wrong type provied for tmQtree_addObj()");

  in_bbox = IN_ON_BBOX(xy, qtree->xy_min, qtree->xy_max);

  if (in_bbox == FALSE)
    return FALSE;

  /*-------------------------------------------------------
  | Remove obj from children if qtree is already splitted
  -------------------------------------------------------*/
  if (qtree->is_splitted == TRUE)
  {
    tmBool removed = tmQtree_remObj(qtree->child_NE, obj);
    if ( removed == FALSE )
      removed = tmQtree_remObj(qtree->child_NW, obj);
    if ( removed == FALSE )
      removed = tmQtree_remObj(qtree->child_SW, obj);
    if ( removed == FALSE )
      removed = tmQtree_remObj(qtree->child_SE, obj);
    if ( removed == FALSE )
      log_err("Failed to remove object from tmQtree.");

    /*-----------------------------------------------------
    | Get number of objects contained in all children
    | and decide if parent can be merged 
    -----------------------------------------------------*/
    int n_obj = tmQtree_getObjNo(qtree);
    if (n_obj <= qtree->max_obj)
      tmQtree_merge(qtree);

    return removed;
  }
  /*-------------------------------------------------------
  | Remove obj from this qtree
  -------------------------------------------------------*/
  else
  {
    /*-----------------------------------------------------
    | Check if object is in this qtree
    -----------------------------------------------------*/
    ListNode *cur;
    tmBool obj_in_qtree = FALSE;

    for (cur = qtree->obj->first; 
         cur != NULL; cur = cur->next)
    {
      if (cur->value == obj)
      {
        obj_in_qtree = TRUE;
        break;
      }
    }

    if (obj_in_qtree == FALSE)
      return FALSE;

    /*-----------------------------------------------------
    | Remove object
    -----------------------------------------------------*/
    List_remove(qtree->obj, cur);
    qtree->n_obj -= 1;

    return TRUE;
  }

} /* tmQtree_remObj() */

/**********************************************************
* Function: tmQtree_getObjNo()
*----------------------------------------------------------
* Return the number of objects contained in this qtree
*----------------------------------------------------------
* @param qtree: tmQtree structure
**********************************************************/
int tmQtree_getObjNo(tmQtree *qtree)
{
  int n_obj = 0;
  
  if ( qtree->is_splitted == TRUE )
  {
    n_obj += tmQtree_getObjNo(qtree->child_NE);
    n_obj += tmQtree_getObjNo(qtree->child_NW);
    n_obj += tmQtree_getObjNo(qtree->child_SW);
    n_obj += tmQtree_getObjNo(qtree->child_SE);
  }
  else
  {
    n_obj += qtree->n_obj;
  }

  return n_obj;

} /* tmQtree_getObjNo() */

/**********************************************************
* Function: tmQtree_containsObj()
*----------------------------------------------------------
* Check if a given objects i contained within this qtree
*----------------------------------------------------------
* @param qtree: tmQtree structure
* @param obj_type: object type of this Qtree
* @param mode: Mode 0 - look only in this qtree
*              Mode 1 - look also in children of this qtree
**********************************************************/
tmBool tmQtree_containsObj(tmQtree *qtree,  
                           void    *obj, 
                           int      mode)
{
  tmBool in_bbox;
  tmDouble *xy;

  /*-------------------------------------------------------
  | Check if object is contained within bbox of qtree
  -------------------------------------------------------*/
  if ( qtree->obj_type == TM_NODE)
    xy = ((tmNode*)obj)->xy;
  else if ( qtree->obj_type == TM_EDGE)
    xy = ((tmEdge*)obj)->xy;
  else if ( qtree->obj_type == TM_TRI)
    xy = ((tmTri*)obj)->xy;
  else
    log_err("Wrong type provied for tmQtree_addObj()");

  in_bbox = IN_ON_BBOX(xy, qtree->xy_min, qtree->xy_max);

  if (in_bbox == FALSE)
    return FALSE;

  /*-------------------------------------------------------
  | Check if object is in children of this qtree
  -------------------------------------------------------*/
  if ( (qtree->is_splitted == TRUE) && (mode == 1) )
  {
    if (tmQtree_containsObj(qtree->child_NE, obj, mode) == TRUE)
      return TRUE;
    if (tmQtree_containsObj(qtree->child_NW, obj, mode) == TRUE)
      return TRUE;
    if (tmQtree_containsObj(qtree->child_SW, obj, mode) == TRUE)
      return TRUE;
    if (tmQtree_containsObj(qtree->child_SE, obj, mode) == TRUE)
      return TRUE;
  }
  /*-------------------------------------------------------
  | Check if object is in this qtree
  -------------------------------------------------------*/
  else
  {
    ListNode *cur;
    tmBool obj_in_qtree = FALSE;

    for (cur = qtree->obj->first; 
         cur != NULL; cur = cur->next)
    {
      if (cur->value == obj)
      {
        obj_in_qtree = TRUE;
        break;
      }
    }
    
    return obj_in_qtree;
  }

} /* tmQtree_hasObj() */
