#include "tg_segmentnetwork.hxx"
#include "tg_polygon.hxx"
#include "tg_shapefile.hxx"

#include "tg_intersection_generator.hxx"

void tgIntersectionGenerator::Insert( const SGGeod& s, const SGGeod& e, double w, int z, unsigned int t )
{    
    segNet.Add( s, e, w, z, t );
}

void tgIntersectionGenerator::ToShapefile( const char* prefix )
{
    char layer[64];
    char name[32];
    
    sprintf( layer, "%s_edges", prefix );
    for (tgintersectionedge_it it = edgelist.begin(); it != edgelist.end(); it++) {
        tgSegment seg( (*it)->start->GetPosition(), (*it)->end->GetPosition() );
        sprintf( name, "seg_%06ld", (*it)->id );
        tgShapefile::FromSegment( seg, true, datasource, layer, name );    
    }
    
    sprintf( layer, "%s_endpoints", prefix );
    for (unsigned int i=0; i<nodelist.size(); i++) {
        if ( nodelist[i]->IsEndpoint() ) {
            sprintf( name, "endpoint_%06d", i );
            tgShapefile::FromGeod( nodelist[i]->GetPosition(), datasource, layer, name );    
        }
    }
}

void tgIntersectionGenerator::Execute( void )
{
    if ( !segNet.empty() ) {
        // Segnet has all of the edges and nodes in an arrangement : clean it
        SG_LOG(SG_GENERAL, SG_INFO, "tgIntersectionGenerator::Clean network " );
        edgelist.clear();
    
        // create a clean segment network
        segNet.Execute();
        
        // now retreive the cleaned edges 
        for ( segnetedge_it snit = segNet.output_begin(); snit != segNet.output_end(); snit++ ) {
            // get the start and end nodes
            tgIntersectionNode* start = nodelist.Add( snit->start );
            tgIntersectionNode* end   = nodelist.Add( snit->end );

            if ( start != end ) {
                tgIntersectionEdge* newEdge = new tgIntersectionEdge( start, end, snit->width, snit->zorder, snit->type, debugRoot );
                edgelist.push_back( newEdge );
            }
        }
        
#if DEBUG_INTERSECTIONS
        SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionGenerator::Saving Cleaned Network to " << debugRoot );
        ToShapefile("cleaned");
#endif
        
        // add end cap segments
        SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionGenerator::Execute:AddCaps");
        for (unsigned int i=0; i<nodelist.size(); i++) {
            SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionGenerator::Execute: ConstrainEdges at node " << i << " of " << nodelist.size() );
            nodelist[i]->AddCapEdges( nodelist, edgelist );
        }
        
#if DEBUG_INTERSECTIONS
        SG_LOG(SG_GENERAL, SG_INFO, "tgIntersectionGenerator::Saving Capped Network to " << debugRoot );
        ToShapefile("Capped");
#endif
        
        //find shared edge constraints at each node
        SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionGenerator::Execute:AddConstraints");
        for (unsigned int i=0; i<nodelist.size(); i++) {
            SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionGenerator::Execute: ConstrainEdges at node " << i << " of " << nodelist.size() );
            nodelist[i]->ConstrainEdges();
        }

        // Generate the edge from each node
        SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionGenerator::Execute:GenerateEdges");
        for (unsigned int i=0; i<nodelist.size(); i++) {
            SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionGenerator::Execute: GenerateEdges at node " << i << " of " << nodelist.size() );
            nodelist[i]->GenerateEdges();
        }

        // Remove any edges that didn't get intersected
        // verifty all edges have been intersected
        tgintersectionedge_it it = edgelist.begin();
        while ( it != edgelist.end() ) {
            if ( !(*it)->Verify( FLAGS_INTERSECT_CONSTRAINTS_COMPLETE) ) {
                (*it)->start->DelEdge( true, *it );
                (*it)->end->DelEdge( false, *it );
                it = edgelist.erase( it );
            } else {
                it++;
            }
        }
            
        // fix multisegment intersections - happens when segments do not diverge from each other before the end of the edge
        SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionGenerator::FixMultiSegment");
        for (unsigned int i=0; i<nodelist.size(); i++) {
            SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionGenerator::Execute: FixSpecialIntersections at node " << i << " of " << nodelist.size() );
            nodelist[i]->CompleteSpecialIntersections();
        }
        
        // verifty all edges have been intersected
        for (tgintersectionedge_it it = edgelist.begin(); it != edgelist.end(); it++) {
            (*it)->Verify( FLAGS_INTERSECT_CONSTRAINTS_COMPLETE);
        }
        
        // complete the edges
        SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionGenerator::CompletePolygon");
        for (tgintersectionedge_it it = edgelist.begin(); it != edgelist.end(); it++) {
            (*it)->Complete();
        }
        
        // to texture, start at caps, and try to cross intersections nicely ( push the start_v )
        SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionGenerator::Texture");        

        // To texture, first find all nodes that contain the repeat endpoints.
        for (unsigned int i=0; i<nodelist.size(); i++) {
            nodelist[i]->CheckEndpoint();
        }

#if DEBUG_TEXTURE        
        ToShapefile("Endpoints" );
#endif

        int num_ep = 0;
        for (unsigned int i=0; i<nodelist.size(); i++) {
            if ( nodelist[i]->IsEndpoint() ) {
                num_ep++;
            }
        }
        
        // Now we traverse between all endpoints
        for (unsigned int i=0; i<nodelist.size(); i++) {
            if ( nodelist[i]->IsEndpoint() ) {
                SG_LOG(SG_GENERAL, LOG_TEXTURE, "tgIntersectionGenerator::Texture ep " << i << " num_eps is " << num_ep );    
                nodelist[i]->TextureEdges( texInfoCb );
            }
        }

        for (tgintersectionedge_it it = edgelist.begin(); it != edgelist.end(); it++) {
            (*it)->Verify( FLAGS_TEXTURED );
        }

        if ( flags & IG_DEBUG_COMPLETE ) {
            SG_LOG(SG_GENERAL, SG_INFO, "Saving complete to " << datasource << " 0%" );    
            unsigned int fivePercent = edgelist.size()/20;
            unsigned int curPercent = fivePercent;
            unsigned int curPrint   = 5;
            
            for (tgintersectionedge_it it = edgelist.begin(); it != edgelist.end(); it++) {
                char feat[32];
                tgPolygon poly = (*it)->GetPoly("complete");
                sprintf(feat, "edge_%06u", poly.GetId() );
                tgShapefile::FromPolygon( poly, false, false, datasource, "complete", feat );

//                (*it)->ToShapefile();
                
                curPercent--;
                if ( curPercent == 0 ) {
                    SG_LOG(SG_GENERAL, SG_INFO, "Saving complete to " << datasource << " " << curPrint << "%" );
                    curPercent = fivePercent;
                    curPrint += 5;
                }
            }
        }
        
        SG_LOG(SG_GENERAL, SG_ALERT, "tgIntersectionGenerator::Complete");    
    }
}