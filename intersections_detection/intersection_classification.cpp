/*****************************************************************************************
 *              MIT License                                                              *
 *                                                                                       *
 * Copyright (c) 2020 Gianmarco Cherchi, Marco Livesu, Riccardo Scateni e Marco Attene   *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION     *
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE        *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                *
 *                                                                                       *
 * Authors:                                                                              *
 *      Gianmarco Cherchi (g.cherchi@unica.it)                                           *
 *      https://people.unica.it/gianmarcocherchi/                                        *
 *                                                                                       *
 *      Marco Livesu (marco.livesu@ge.imati.cnr.it)                                      *
 *      http://pers.ge.imati.cnr.it/livesu/                                              *
 *                                                                                       *
 *      Riccardo Scateni (riccardo@unica.it)                                             *
 *      https://people.unica.it/riccardoscateni/                                         *
 *                                                                                       *
 *      Marco Attene (marco.attene@ge.imati.cnr.it)                                      *
 *      https://www.cnr.it/en/people/marco.attene/                                       *
 *                                                                                       *
 * ***************************************************************************************/

#include "intersection_classification.h"

using namespace cinolib;

void classifyIntersections(TriangleSoup &ts, AuxiliaryStructure &g)
{
    for(uint tA_id = 0; tA_id < g.intersectionList().size(); tA_id++)
    {
        for(const uint &tB_id : g.triangleIntersectionList(tA_id))
        {
            if(tA_id < tB_id)   //we check the intersection between t1 and t2 only once
            {
                checkTriangleTriangleIntersections(ts, g, tA_id, tB_id);
            }
        }
    }

    // Coplanar triangles intersections propagation
    propagateCoplanarTrianlesIntersections(ts, g);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

void checkTriangleTriangleIntersections(TriangleSoup &ts, AuxiliaryStructure &g, const uint &tA_id, const uint &tB_id)
{
    std::set<uint> v_tmp; // temporary vtx list for final symbolic edge creation
    bool coplanar_tris = false;
    std::set<uint> li; // intersection list

    /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
     *      check of tB respect to tA
     * :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
    double orBA[3];
    orBA[0] = orient3d(ts.triVertPtr(tB_id, 0), ts.triVertPtr(tA_id, 0), ts.triVertPtr(tA_id, 1), ts.triVertPtr(tA_id, 2));
    orBA[1] = orient3d(ts.triVertPtr(tB_id, 1), ts.triVertPtr(tA_id, 0), ts.triVertPtr(tA_id, 1), ts.triVertPtr(tA_id, 2));
    orBA[2] = orient3d(ts.triVertPtr(tB_id, 2), ts.triVertPtr(tA_id, 0), ts.triVertPtr(tA_id, 1), ts.triVertPtr(tA_id, 2));
    normalizeOrientations(orBA);

    if(sameOrientation(orBA[0], orBA[1]) && sameOrientation(orBA[1], orBA[2]) && (orBA[0] != 0.0)) return;   //no intersection found

    // all edge of tB are coplanar to all edges of tA   (orBA: 0 0 0)
    if(allCoplanarEdges(orBA))
    {
        g.addCoplanarTriangles(tA_id, tB_id);
        coplanar_tris = true;

        checkSingleCoplanarEdgeIntersections(ts, ts.triVertID(tB_id, 0), ts.triVertID(tB_id, 1), tB_id, tA_id, g, li);
        checkSingleCoplanarEdgeIntersections(ts, ts.triVertID(tB_id, 1), ts.triVertID(tB_id, 2), tB_id, tA_id, g, li);
        checkSingleCoplanarEdgeIntersections(ts, ts.triVertID(tB_id, 2), ts.triVertID(tB_id, 0), tB_id, tA_id, g, li);
    }

    // a single edge of tB is coplanar to tA    (e.g. orBA: 1 0 0)
    int tmp_edge_id = singleCoplanarEdge(orBA);
    if(tmp_edge_id != -1)
    {
        uint e_v0_id = static_cast<uint>(tmp_edge_id);
        uint e_v1_id = (tmp_edge_id + 1) % 3;
        checkSingleCoplanarEdgeIntersections(ts, ts.triVertID(tB_id, e_v0_id), ts.triVertID(tB_id, e_v1_id), tB_id, tA_id, g, li);
    }

    // a vertex of tB is coplanar to tA, and the opposite edge is on the same side respect to tA  (e.g. orBA: 1 0 1)
    int tmp_vtx_id = vtxInPlaneAndOppositeEdgeOnSameSide(orBA);
    if(tmp_vtx_id != -1)
        checkVtxInTriangleIntersection(ts, ts.triVertID(tB_id, static_cast<uint>(tmp_vtx_id)), tA_id, v_tmp, g, li);

    // a vertex of tB is coplanar to tA, and the opposite edge could intersect tA   (e.g. orBA: -1 0 1)
    tmp_vtx_id = vtxInPlaneAndOppositeEdgeCrossPlane(orBA);
    if(tmp_vtx_id != -1)
    {
        uint real_v_id = ts.triVertID(tB_id, static_cast<uint>(tmp_vtx_id));
        checkVtxInTriangleIntersection(ts, real_v_id, tA_id, v_tmp, g, li);

        uint opp_edge_id = ts.edgeOppositeToVert(tB_id, ts.triVertID(tB_id, static_cast<uint>(tmp_vtx_id)));
        checkSingleNoCoplanarEdgeIntersection(ts, opp_edge_id, tA_id, v_tmp, g, li);
    }

    // a vertex of tB is on one side of the plane defined to tA, and the opposite edge (always in tB) is in the other (e.g. orBA: -1 1 1)
    uint opp_v0, opp_v1;
    tmp_vtx_id = vtxOnASideAndOppositeEdgeOnTheOther(orBA, opp_v0, opp_v1);
    if(tmp_vtx_id != -1)
    {
        uint id_v = ts.triVertID(tB_id, static_cast<uint>(tmp_vtx_id));
        uint id_opp_v0 = ts.triVertID(tB_id, opp_v0);
        uint id_opp_v1 = ts.triVertID(tB_id, opp_v1);

        int edge_id0 = ts.edgeID(id_v, id_opp_v0);
        int edge_id1 = ts.edgeID(id_v, id_opp_v1);
        assert(edge_id0 != -1 && edge_id1 != -1);

        checkSingleNoCoplanarEdgeIntersection(ts, static_cast<uint>(edge_id0), tA_id, v_tmp, g, li);
        checkSingleNoCoplanarEdgeIntersection(ts, static_cast<uint>(edge_id1), tA_id, v_tmp, g, li);
    }

    if(!coplanar_tris && li.size() > 1) goto final_check; // sorry about that :(

    /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
     *      check of A respect to B
     * :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */

    double orAB[3];

    // all edge of tA are coplanar to all edges of tB   (orAB: 0 0 0)
    if(coplanar_tris)
    {
        orAB[0] = 0; orAB[1] = 0; orAB[2] = 0;
        checkSingleCoplanarEdgeIntersections(ts, ts.triVertID(tA_id, 0), ts.triVertID(tA_id, 1), tA_id, tB_id, g, li);
        checkSingleCoplanarEdgeIntersections(ts, ts.triVertID(tA_id, 1), ts.triVertID(tA_id, 2), tA_id, tB_id, g, li);
        checkSingleCoplanarEdgeIntersections(ts, ts.triVertID(tA_id, 2), ts.triVertID(tA_id, 0), tA_id, tB_id, g, li);
    }
    else
    {
        orAB[0] = orient3d(ts.triVertPtr(tA_id, 0), ts.triVertPtr(tB_id, 0), ts.triVertPtr(tB_id, 1), ts.triVertPtr(tB_id, 2));
        orAB[1] = orient3d(ts.triVertPtr(tA_id, 1), ts.triVertPtr(tB_id, 0), ts.triVertPtr(tB_id, 1), ts.triVertPtr(tB_id, 2));
        orAB[2] = orient3d(ts.triVertPtr(tA_id, 2), ts.triVertPtr(tB_id, 0), ts.triVertPtr(tB_id, 1), ts.triVertPtr(tB_id, 2));
        normalizeOrientations(orAB);

        if(sameOrientation(orAB[0], orAB[1]) && sameOrientation(orAB[1], orAB[2]) && (orAB[0] != 0.0)) return;   //no intersection
    }

    // a single edge of tA is coplanar to tB    (e.g. orAB: 1 0 0)
    tmp_edge_id = singleCoplanarEdge(orAB);
    if(tmp_edge_id != -1)
    {
        uint e_v0_id = static_cast<uint>(tmp_edge_id);
        uint e_v1_id = (tmp_edge_id + 1) % 3;
        checkSingleCoplanarEdgeIntersections(ts, ts.triVertID(tA_id, e_v0_id), ts.triVertID(tA_id, e_v1_id), tA_id, tB_id, g, li);
    }

    // a vertex of tA is coplanar to tB, and the opposite edge is on the same side respect to tB  (e.g. orAB: 1 0 1)
    tmp_vtx_id = vtxInPlaneAndOppositeEdgeOnSameSide(orAB);
    if(tmp_vtx_id != -1)
        checkVtxInTriangleIntersection(ts, ts.triVertID(tA_id, static_cast<uint>(tmp_vtx_id)), tB_id, v_tmp, g, li);

    // a vertex of tA is coplanar to tB, and the opposite edge could intersect tB   (e.g. orAB: -1 0 1)
    tmp_vtx_id = vtxInPlaneAndOppositeEdgeCrossPlane(orAB);
    if(tmp_vtx_id != -1)
    {
        uint real_v_id = ts.triVertID(tA_id, static_cast<uint>(tmp_vtx_id));
        checkVtxInTriangleIntersection(ts, real_v_id, tB_id, v_tmp, g, li);

        uint opp_edge_id = ts.edgeOppositeToVert(tA_id, ts.triVertID(tA_id, static_cast<uint>(tmp_vtx_id)));
        checkSingleNoCoplanarEdgeIntersection(ts, opp_edge_id, tB_id, v_tmp, g, li);
    }

    // a vertex of tA is on one side of the plane defined to tB, and the opposite edge (always in tA) is in the other (e.g. orBA: -1 1 1)
    tmp_vtx_id = vtxOnASideAndOppositeEdgeOnTheOther(orAB, opp_v0, opp_v1);
    if(tmp_vtx_id != -1)
    {
        uint id_v = ts.triVertID(tA_id, static_cast<uint>(tmp_vtx_id));
        uint id_opp_v0 = ts.triVertID(tA_id, opp_v0);
        uint id_opp_v1 = ts.triVertID(tA_id, opp_v1);

        int edge_id0 = ts.edgeID(id_v, id_opp_v0);
        int edge_id1 = ts.edgeID(id_v, id_opp_v1);
        assert(edge_id0 != -1 && edge_id1 != -1);

        checkSingleNoCoplanarEdgeIntersection(ts, static_cast<uint>(edge_id0), tB_id, v_tmp, g, li);
        checkSingleNoCoplanarEdgeIntersection(ts, static_cast<uint>(edge_id1), tB_id, v_tmp, g, li);
    }

    final_check:

    g.incrementNumIntersections(static_cast<uint>(li.size()));

    if(coplanar_tris)
        assert(v_tmp.size() <= 3 && "more than 3 intersection points in coplanar triangles");
    else
        assert((!coplanar_tris && v_tmp.size() <= 2) && "more than 2 intersection points in 2 no-coplanar traingles");

    if(v_tmp.size() == 2)
    {
        uint v0_id = *(v_tmp.begin());
        uint v1_id = *(++v_tmp.begin());
        addNewIntersection(ts, SYMBOLIC_SEGMENT, {v0_id, v1_id, tA_id, tB_id}, g);
    }

}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

uint addNewIntersection(TriangleSoup &ts, const Intersection &i, const std::vector<uint> &int_elems, AuxiliaryStructure &g)
{
    switch (i)
    {
        case VTX_IN_TRIANGLE:  // no new vertex is generated
        {
            assert(int_elems.size() == 2);
            uint v_id = int_elems[0];
            uint t_id = int_elems[1];

            g.addVertexInTriangle(t_id, v_id);
            return v_id;
        }

        case VTX_IN_EDGE:   // no new vertex is generated
        {
            assert(int_elems.size() == 2);
            uint v_id = int_elems[0];
            uint e_id = int_elems[1];

            g.addVertexInEdge(e_id, v_id);
            return v_id;
        }

        case EDGE_CROSS_EDGE:
        {
            uint e_id0 = int_elems[0];
            uint e_id1 = int_elems[1];
            implicitPoint3D_LPI *tmp_i = nullptr;

            if(int_elems.size() == 2) // no triangle id passed
            {

                uint jolly_id = noCoplanarJollyPointID(ts, ts.edgeVertPtr(e_id1, 0),
                                                           ts.edgeVertPtr(e_id1, 1),
                                                           ts.edgeVertPtr(e_id0, 0));

                tmp_i = new implicitPoint3D_LPI(ts.edgeVert(e_id0, 0)->toExplicit3D(),
                                                ts.edgeVert(e_id0, 1)->toExplicit3D(),
                                                ts.edgeVert(e_id1, 0)->toExplicit3D(),
                                                ts.edgeVert(e_id1, 1)->toExplicit3D(),
                                                ts.jollyPoint(jolly_id)->toExplicit3D());
            }
            else if(int_elems.size() == 3) // triangle id passed
            {
                uint t_id = int_elems[2];

                tmp_i = new implicitPoint3D_LPI(ts.edgeVert(e_id0, 0)->toExplicit3D(),
                                                ts.edgeVert(e_id0, 1)->toExplicit3D(),
                                                ts.triVert(t_id, 0)->toExplicit3D(),
                                                ts.triVert(t_id, 1)->toExplicit3D(),
                                                ts.triVert(t_id, 2)->toExplicit3D());
            }

            uint pos = ts.numVerts();
            uint new_v_id;
            std::pair<uint, bool> ins = g.addVertexInSortedList(std::make_pair(tmp_i, pos)); // check if the intersection already exists

            if(ins.second) // new vertex
            {
                double x, y, z;
                assert(tmp_i->getApproxXYZCoordinates(x, y, z) && "LPI point badly formed");

                new_v_id = ts.addVert(tmp_i); // add new vertex in mesh
                assert(new_v_id == pos);
            }
            else // already present vertex
            {
                new_v_id = ins.first;
                delete tmp_i;
            }

            g.addVertexInEdge(e_id0, new_v_id);
            g.addVertexInEdge(e_id1, new_v_id);
            return new_v_id;
        }

        case EDGE_CROSS_TRIANGLE:
        {
            assert(int_elems.size() == 2);
            uint e_id = int_elems[0];
            uint t_id = int_elems[1];

            implicitPoint3D_LPI *tmp_i = new implicitPoint3D_LPI(ts.edgeVert(e_id, 0)->toExplicit3D(),
                                                                 ts.edgeVert(e_id, 1)->toExplicit3D(),
                                                                 ts.triVert(t_id, 0)->toExplicit3D(),
                                                                 ts.triVert(t_id, 1)->toExplicit3D(),
                                                                 ts.triVert(t_id, 2)->toExplicit3D());
            uint pos = ts.numVerts();
            uint new_v_id;
            std::pair<uint, bool> ins = g.addVertexInSortedList(std::make_pair(tmp_i, pos)); // check if the intersection already exists

            if(ins.second) // new vertex
            {
                double x, y, z;
                assert(tmp_i->getApproxXYZCoordinates(x, y, z) && "LPI point badly formed");

                new_v_id = ts.addVert(tmp_i);   // add vertex in mesh
                assert(new_v_id == pos);
            }
            else // already present vertex
            {
                new_v_id = ins.first;
                delete tmp_i;
            }

            g.addVertexInTriangle(t_id, new_v_id);
            g.addVertexInEdge(e_id, new_v_id);
            return new_v_id;
        }

        case SYMBOLIC_SEGMENT:
        {
            assert(int_elems.size() == 4);
            uint v0_id = int_elems[0];
            uint v1_id = int_elems[1];
            uint tA_id = int_elems[2];
            uint tB_id = int_elems[3];

            assert(v0_id != v1_id && "trying to add a 0-lenght symbolic edge");

            UIPair segment = std::make_pair(v0_id, v1_id);

            if(!ts.triContainsEdge(tA_id, v0_id, v1_id))
                g.addSegmentInTriangle(tA_id, segment);

            if(!ts.triContainsEdge(tB_id, v0_id, v1_id))
                g.addSegmentInTriangle(tB_id, segment);

            std::vector<uint> ref_tris = {tA_id, tB_id};
            g.addTrianglesInSegment(segment, ref_tris);

            return 0; // nothing to return, the edge is not added to the mesh yet
        }

        case THREE_TRIANGLES_CROSS:
        {
            // NEVER CALLED: we add the TPI points in the addConstraintSegments (during the triangulation)
            assert(false && "trying to add a TPI point in the wrong place");
            return 0; // warning killer
        }

        case NONE: break;

    }

    assert(false && "generic intersection this should not happen");
    return 0; // warning killer
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

uint noCoplanarJollyPointID(const TriangleSoup &ts, const double *v0, const double *v1, const double *v2)
{
    for(uint jp_id = 0; jp_id < 4; jp_id++) // we are looking for a joppy point not aligned with the triangle
    {
        double jp[3] = {ts.jollyPoint(jp_id)->toExplicit3D().X(),
                        ts.jollyPoint(jp_id)->toExplicit3D().Y(),
                        ts.jollyPoint(jp_id)->toExplicit3D().Z()};

        if(cinolib::orient3d(v0, v1, v2, jp) != 0.0)
            return jp_id;
    }

    assert(false && "no suitable jolly point found"); // impossible
    return 0; // warning killer
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

void checkSingleCoplanarEdgeIntersections(TriangleSoup &ts, const uint &e_v0, const uint &e_v1,
                                          const uint &e_t_id, const uint &o_t_id,
                                          AuxiliaryStructure &g, std::set<uint> &il) // il -> intersection list
{
    bool  v0_in_vtx = false,    v1_in_vtx = false;
    int  v0_in_seg = -1,        v1_in_seg = -1;
    bool v0_in_tri = false,     v1_in_tri = false;


    // e_v0 position
    PointInSimplex v0_inters = point_in_triangle_3d(ts.vertPtr(e_v0), ts.triVertPtr(o_t_id, 0), ts.triVertPtr(o_t_id, 1), ts.triVertPtr(o_t_id, 2));
    if(v0_inters == ON_VERT0 || v0_inters == ON_VERT1 || v0_inters == ON_VERT2)
    {
        v0_in_vtx = true; // v0 in a vertex
        il.insert(e_v0);
    }
    else if(v0_inters == ON_EDGE0) v0_in_seg = static_cast<int>(ts.triEdgeID(o_t_id, 0)); // v0 in seg0
    else if(v0_inters == ON_EDGE1) v0_in_seg = static_cast<int>(ts.triEdgeID(o_t_id, 1)); // v0 in seg1
    else if(v0_inters == ON_EDGE2) v0_in_seg = static_cast<int>(ts.triEdgeID(o_t_id, 2)); // v0 in seg2
    else if(v0_inters == STRICTLY_INSIDE) v0_in_tri = true; // v0 inside tri


    // e_v1 position
    PointInSimplex v1_inters = point_in_triangle_3d(ts.vertPtr(e_v1), ts.triVertPtr(o_t_id, 0), ts.triVertPtr(o_t_id, 1), ts.triVertPtr(o_t_id, 2));
    if(v1_inters == ON_VERT0 || v1_inters == ON_VERT1 || v1_inters == ON_VERT2)
    {
        v1_in_vtx = true; // v1 in a vertex
        il.insert(e_v1);
    }
    else if(v1_inters == ON_EDGE0) v1_in_seg = static_cast<int>(ts.triEdgeID(o_t_id, 0)); // v1 in seg0
    else if(v1_inters == ON_EDGE1) v1_in_seg = static_cast<int>(ts.triEdgeID(o_t_id, 1)); // v1 in seg1
    else if(v1_inters == ON_EDGE2) v1_in_seg = static_cast<int>(ts.triEdgeID(o_t_id, 2)); // v1 in seg2
    else if(v1_inters == STRICTLY_INSIDE) v1_in_tri = true; // v1 inside tri

    if(v0_in_vtx && v1_in_vtx) return;

    if(v0_in_seg != -1 && v1_in_seg != -1)  //edge in triangle composed by the link of two vtx in edge
    {
        addNewIntersection(ts, VTX_IN_EDGE, {e_v0, static_cast<uint>(v0_in_seg)}, g);
        addNewIntersection(ts, VTX_IN_EDGE, {e_v1, static_cast<uint>(v1_in_seg)}, g);
        il.insert(e_v0);
        il.insert(e_v1);

        addNewIntersection(ts, SYMBOLIC_SEGMENT, {e_v0, e_v1, e_t_id, o_t_id}, g);
        return;
    }
    else if(v0_in_seg != -1) // only v0 is in a segment of T
    {
        addNewIntersection(ts, VTX_IN_EDGE, {e_v0, static_cast<uint>(v0_in_seg)}, g);
        il.insert(e_v0);

        if(v1_in_vtx)
        {
            addNewIntersection(ts, SYMBOLIC_SEGMENT, {e_v0, e_v1, e_t_id, o_t_id}, g);
            return;
        }
    }
    else if(v1_in_seg != -1) // only v1 is in a segment of T
    {
        addNewIntersection(ts, VTX_IN_EDGE, {e_v1, static_cast<uint>(v1_in_seg)}, g);
        il.insert(e_v1);

        if(v0_in_vtx)
        {
            addNewIntersection(ts, SYMBOLIC_SEGMENT, {e_v1, e_v0, e_t_id, o_t_id}, g);
            return;
        }
    }

    // v0 in a segment or vtx and v1 inside triangle
    if((v0_in_seg != -1 || v0_in_vtx) && v1_in_tri)
    {
        addNewIntersection(ts, VTX_IN_TRIANGLE, {e_v1, o_t_id}, g);
        il.insert(e_v1);

        addNewIntersection(ts, SYMBOLIC_SEGMENT, {e_v0, e_v1, e_t_id, o_t_id}, g);
        return;
    }

    // v1 in a segment or vtx and v0 inside triangle
    if((v1_in_seg != -1 || v1_in_vtx) && v0_in_tri)
    {
        addNewIntersection(ts, VTX_IN_TRIANGLE, {e_v0, o_t_id}, g);
        il.insert(e_v0);

        addNewIntersection(ts, SYMBOLIC_SEGMENT, {e_v0, e_v1, e_t_id, o_t_id}, g);
        return;
    }

    // v0 and v1 both inside the triangle
    if(v0_in_tri && v1_in_tri)
    {
        addNewIntersection(ts, VTX_IN_TRIANGLE, {e_v0, o_t_id}, g);
        addNewIntersection(ts, VTX_IN_TRIANGLE, {e_v1, o_t_id}, g);
        il.insert(e_v0);
        il.insert(e_v1);

        addNewIntersection(ts, SYMBOLIC_SEGMENT, {e_v0, e_v1, e_t_id, o_t_id}, g);
        return;
    }

    if(v0_in_tri)  // only v0 inside the triangle
    {
        addNewIntersection(ts, VTX_IN_TRIANGLE, {e_v0, o_t_id}, g);
        il.insert(e_v0);
    }
    else if(v1_in_tri)  // only v1 inside the triangle
    {
        addNewIntersection(ts, VTX_IN_TRIANGLE, {e_v1, o_t_id}, g);
        il.insert(e_v1);
    }

    // Edges cross checking

    // we check only if seg A cross seg B and not B cross A (we found the intersection once)
    int o_t_e0 = static_cast<int>(ts.triEdgeID(o_t_id, 0));
    int o_t_e1 = static_cast<int>(ts.triEdgeID(o_t_id, 1));
    int o_t_e2 = static_cast<int>(ts.triEdgeID(o_t_id, 2));

    bool tv0_in_edge = point_in_segment_3d(ts.triVertPtr(o_t_id, 0), ts.vertPtr(e_v0), ts.vertPtr(e_v1)) != STRICTLY_OUTSIDE;
    bool tv1_in_edge = point_in_segment_3d(ts.triVertPtr(o_t_id, 1), ts.vertPtr(e_v0), ts.vertPtr(e_v1)) != STRICTLY_OUTSIDE;
    bool tv2_in_edge = point_in_segment_3d(ts.triVertPtr(o_t_id, 2), ts.vertPtr(e_v0), ts.vertPtr(e_v1)) != STRICTLY_OUTSIDE;

    int seg0_cross = -1, seg1_cross = -1, seg2_cross = -1;
    int curr_e_id = ts.edgeID(e_v0, e_v1);

    if(v0_in_seg != o_t_e0 && v1_in_seg != o_t_e0 && !tv0_in_edge && !tv1_in_edge &&
       segment_segment_intersect_3d(ts.vertPtr(e_v0), ts.vertPtr(e_v1), ts.triVertPtr(o_t_id, 0), ts.triVertPtr(o_t_id, 1)) == INTERSECT &&
       point_in_segment_3d(ts.triVertPtr(o_t_id, 0), ts.vertPtr(e_v0), ts.vertPtr(e_v1)) == STRICTLY_OUTSIDE &&
       point_in_segment_3d(ts.triVertPtr(o_t_id, 1), ts.vertPtr(e_v0), ts.vertPtr(e_v1)) == STRICTLY_OUTSIDE) // edge e cross seg 0
    {
        seg0_cross = static_cast<int>(addNewIntersection(ts, EDGE_CROSS_EDGE, {static_cast<uint>(o_t_e0), static_cast<uint>(curr_e_id)}, g));
        il.insert(static_cast<uint>(seg0_cross));

        if(v0_in_vtx || v0_in_seg != -1 || v0_in_tri)
        {
            addNewIntersection(ts, SYMBOLIC_SEGMENT, {e_v0, static_cast<uint>(seg0_cross), e_t_id, o_t_id}, g);
            return;
        }
        else if(v1_in_vtx || v1_in_seg != -1 || v1_in_tri)
        {
            addNewIntersection(ts, SYMBOLIC_SEGMENT, {e_v1, static_cast<uint>(seg0_cross), e_t_id, o_t_id}, g);
            return;
        }
        else if(tv2_in_edge)
        {
            addNewIntersection(ts, SYMBOLIC_SEGMENT, {ts.triVertID(o_t_id, 2), static_cast<uint>(seg0_cross), o_t_id, e_t_id}, g);
            il.insert(ts.triVertID(o_t_id, 2));
            return;
        }
    }

    if(v0_in_seg != o_t_e1 && v1_in_seg != o_t_e1 && !tv1_in_edge && !tv2_in_edge &&
       segment_segment_intersect_3d(ts.vertPtr(e_v0), ts.vertPtr(e_v1), ts.triVertPtr(o_t_id, 1), ts.triVertPtr(o_t_id, 2)) == INTERSECT &&
       point_in_segment_3d(ts.triVertPtr(o_t_id, 1), ts.vertPtr(e_v0), ts.vertPtr(e_v1)) == STRICTLY_OUTSIDE &&
       point_in_segment_3d(ts.triVertPtr(o_t_id, 2), ts.vertPtr(e_v0), ts.vertPtr(e_v1)) == STRICTLY_OUTSIDE) // edge e cross seg 1
    {
        seg1_cross = static_cast<int>(addNewIntersection(ts, EDGE_CROSS_EDGE, {static_cast<uint>(o_t_e1), static_cast<uint>(curr_e_id)}, g));
        il.insert(static_cast<uint>(seg1_cross));

        if(v0_in_vtx || v0_in_seg != -1 || v0_in_tri)
        {
            addNewIntersection(ts, SYMBOLIC_SEGMENT, {e_v0, static_cast<uint>(seg1_cross), e_t_id, o_t_id}, g);
            return;
        }
        else if(v1_in_vtx || v1_in_seg != -1 || v1_in_tri)
        {
            addNewIntersection(ts, SYMBOLIC_SEGMENT, {e_v1, static_cast<uint>(seg1_cross), e_t_id, o_t_id}, g);
            return;
        }
        else if(tv0_in_edge)
        {
            addNewIntersection(ts, SYMBOLIC_SEGMENT, {ts.triVertID(o_t_id, 0), static_cast<uint>(seg1_cross), o_t_id, e_t_id}, g);
            il.insert(ts.triVertID(o_t_id, 0));
            return;
        }
    }

    if(v0_in_seg != o_t_e2 && v1_in_seg != o_t_e2 && !tv2_in_edge && !tv0_in_edge &&
       segment_segment_intersect_3d(ts.vertPtr(e_v0), ts.vertPtr(e_v1), ts.triVertPtr(o_t_id, 2), ts.triVertPtr(o_t_id, 0)) == INTERSECT &&
       point_in_segment_3d(ts.triVertPtr(o_t_id, 2), ts.vertPtr(e_v0), ts.vertPtr(e_v1)) == STRICTLY_OUTSIDE &&
       point_in_segment_3d(ts.triVertPtr(o_t_id, 0), ts.vertPtr(e_v0), ts.vertPtr(e_v1)) == STRICTLY_OUTSIDE) // edge e cross seg2
    {
        seg2_cross = static_cast<int>(addNewIntersection(ts, EDGE_CROSS_EDGE, {static_cast<uint>(o_t_e2), static_cast<uint>(curr_e_id)}, g));
        il.insert(static_cast<uint>(seg2_cross));

        if(v0_in_vtx || v0_in_seg != -1 || v0_in_tri)
        {
            addNewIntersection(ts, SYMBOLIC_SEGMENT, {e_v0, static_cast<uint>(seg2_cross), e_t_id, o_t_id}, g);
            return;
        }
        else if(v1_in_vtx || v1_in_seg != -1 || v1_in_tri)
        {
            addNewIntersection(ts, SYMBOLIC_SEGMENT, {e_v1, static_cast<uint>(seg2_cross), e_t_id, o_t_id}, g);
            return;
        }
        else if(tv1_in_edge)
        {
            addNewIntersection(ts, SYMBOLIC_SEGMENT, {ts.triVertID(o_t_id, 1), static_cast<uint>(seg2_cross), o_t_id, e_t_id}, g);
            il.insert(ts.triVertID(o_t_id, 1));
            return;
        }
    }

    // final probably symbolic edges
    if(seg0_cross != -1 && seg1_cross != -1)
        addNewIntersection(ts, SYMBOLIC_SEGMENT, {static_cast<uint>(seg0_cross), static_cast<uint>(seg1_cross), e_t_id, o_t_id}, g);

    else if(seg0_cross != -1 && seg2_cross != -1)
        addNewIntersection(ts, SYMBOLIC_SEGMENT, {static_cast<uint>(seg0_cross), static_cast<uint>(seg2_cross), e_t_id, o_t_id}, g);

    else if(seg1_cross != -1 && seg2_cross != -1)
        addNewIntersection(ts, SYMBOLIC_SEGMENT, {static_cast<uint>(seg1_cross), static_cast<uint>(seg2_cross), e_t_id, o_t_id}, g);

    if(tv0_in_edge)
    {
        if(v0_in_seg != -1 || v0_in_tri) addNewIntersection(ts, SYMBOLIC_SEGMENT, {ts.triVertID(o_t_id, 0), e_v0, o_t_id, e_t_id}, g);
        else if(v1_in_seg != -1 || v1_in_tri) addNewIntersection(ts, SYMBOLIC_SEGMENT, {ts.triVertID(o_t_id, 0), e_v1, o_t_id, e_t_id}, g);
    }

    if(tv1_in_edge)
    {
        if(v0_in_seg != -1 || v0_in_tri) addNewIntersection(ts, SYMBOLIC_SEGMENT, {ts.triVertID(o_t_id, 1), e_v0, o_t_id, e_t_id}, g);
        else if(v1_in_seg != -1 || v1_in_tri) addNewIntersection(ts, SYMBOLIC_SEGMENT, {ts.triVertID(o_t_id, 1), e_v1, o_t_id, e_t_id}, g);
    }

    if(tv2_in_edge)
    {
        if(v0_in_seg != -1 || v0_in_tri) addNewIntersection(ts, SYMBOLIC_SEGMENT, {ts.triVertID(o_t_id, 2), e_v0, o_t_id, e_t_id}, g);
        else if(v1_in_seg != -1 || v1_in_tri) addNewIntersection(ts, SYMBOLIC_SEGMENT, {ts.triVertID(o_t_id, 2), e_v1, o_t_id, e_t_id}, g);
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

void checkSingleNoCoplanarEdgeIntersection(TriangleSoup &ts, const uint &e_id, const uint &t_id,
                                           std::set<uint> &v_tmp, AuxiliaryStructure &g, std::set<uint> &li) // li -> intersection list
{

    SimplexIntersection inters = segment_triangle_intersect_3d(ts.edgeVertPtr(e_id, 0), ts.edgeVertPtr(e_id, 1),
                                                               ts.triVertPtr(t_id, 0), ts.triVertPtr(t_id, 1), ts.triVertPtr(t_id, 2));

    if(inters == DO_NOT_INTERSECT || inters == SIMPLICIAL_COMPLEX) return; // no intersection found

    if(point_in_segment_3d(ts.triVertPtr(t_id, 0), ts.edgeVertPtr(e_id, 0), ts.edgeVertPtr(e_id, 1)) == STRICTLY_INSIDE ||
       point_in_segment_3d(ts.triVertPtr(t_id, 1), ts.edgeVertPtr(e_id, 0), ts.edgeVertPtr(e_id, 1)) == STRICTLY_INSIDE ||
       point_in_segment_3d(ts.triVertPtr(t_id, 2), ts.edgeVertPtr(e_id, 0), ts.edgeVertPtr(e_id, 1)) == STRICTLY_INSIDE)
        return;

    // the edge intersect the tri in seg 0
    if(segment_segment_intersect_3d(ts.edgeVertPtr(e_id, 0), ts.edgeVertPtr(e_id, 1),
                                    ts.triVertPtr(t_id, 0), ts.triVertPtr(t_id, 1)) == INTERSECT)
    {
        uint e_id2 = ts.triEdgeID(t_id, 0);
        uint int_point = addNewIntersection(ts, EDGE_CROSS_EDGE, {e_id, e_id2, t_id}, g);
        li.insert(int_point);
        v_tmp.insert(int_point);
        return ;
    }

    // the edge intersect the tri in seg 1
    if(segment_segment_intersect_3d(ts.edgeVertPtr(e_id, 0), ts.edgeVertPtr(e_id, 1),
                                    ts.triVertPtr(t_id, 1), ts.triVertPtr(t_id, 2)) == INTERSECT)
    {
        uint e_id2 = ts.triEdgeID(t_id, 1);
        uint int_point = addNewIntersection(ts, EDGE_CROSS_EDGE, {e_id, e_id2, t_id}, g);
        li.insert(int_point);
        v_tmp.insert(int_point);
        return ;
    }

    // the edge intersect the tri in seg 2
    if(segment_segment_intersect_3d(ts.edgeVertPtr(e_id, 0), ts.edgeVertPtr(e_id, 1),
                                    ts.triVertPtr(t_id, 2), ts.triVertPtr(t_id, 0)) == INTERSECT)
    {
        uint e_id2 = ts.triEdgeID(t_id, 2);
        uint int_point = addNewIntersection(ts, EDGE_CROSS_EDGE, {e_id, e_id2, t_id}, g);
        li.insert(int_point);
        v_tmp.insert(int_point);
        return ;
    }

    // the edge intersect the inner triangle
    uint int_point = addNewIntersection(ts, EDGE_CROSS_TRIANGLE, {e_id, t_id}, g);
    li.insert(int_point);
    v_tmp.insert(int_point);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

void checkVtxInTriangleIntersection(TriangleSoup &ts, const uint &v_id, const uint &t_id, std::set<uint> &v_tmp, AuxiliaryStructure &g, std::set<uint> &li) // li -> intersection list
{
    PointInSimplex inters = point_in_triangle_3d(ts.vertPtr(v_id), ts.triVertPtr(t_id, 0), ts.triVertPtr(t_id, 1), ts.triVertPtr(t_id, 2));

    switch(inters)
    {
        case STRICTLY_OUTSIDE: return;

        case ON_EDGE0:
        {
            uint e_id = ts.triEdgeID(t_id, 0);
            addNewIntersection(ts, VTX_IN_EDGE,{v_id, e_id}, g);
            li.insert(v_id);
            v_tmp.insert(v_id);
        } break;

        case ON_EDGE1:
        {
            uint e_id = ts.triEdgeID(t_id, 1);
            addNewIntersection(ts, VTX_IN_EDGE,{v_id, e_id}, g);
            li.insert(v_id);
            v_tmp.insert(v_id);
        } break;

        case ON_EDGE2:
        {
            uint e_id = ts.triEdgeID(t_id, 2);
            addNewIntersection(ts, VTX_IN_EDGE,{v_id, e_id}, g);
            li.insert(v_id);
            v_tmp.insert(v_id);
        } break;

        case STRICTLY_INSIDE:
        {
            addNewIntersection(ts, VTX_IN_TRIANGLE, {v_id, t_id}, g);
            li.insert(v_id);
            v_tmp.insert(v_id);
        } break;

        case ON_VERT0:
        case ON_VERT1:
        case ON_VERT2:
        {
            v_tmp.insert(v_id);
            li.insert(v_id);
        } break;
    }

}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

void propagateCoplanarTrianlesIntersections(TriangleSoup &ts, AuxiliaryStructure &g)
{
    for(uint t_id = 0; t_id < ts.numTris(); t_id++)
    {
        if(g.triangleHasCoplanars(t_id))
        {
            // intersection points inside triangle
            for(auto &copl_t : g.coplanarTriangles(t_id))
            {
                uint e0_id = ts.triEdgeID(copl_t, 0);
                uint e1_id = ts.triEdgeID(copl_t, 1);
                uint e2_id = ts.triEdgeID(copl_t, 2);

                for(auto &p_id : g.edgePointsList(e0_id))
                {
                    if(!ts.triContainsVert(t_id, p_id) && genericPointInsideTriangle(ts, p_id, t_id, true))
                        g.addVertexInTriangle(t_id, p_id);
                }

                for(auto &p_id : g.edgePointsList(e1_id))
                {
                    if(!ts.triContainsVert(t_id, p_id) && genericPointInsideTriangle(ts, p_id, t_id, true))
                        g.addVertexInTriangle(t_id, p_id);
                }

                for(auto &p_id : g.edgePointsList(e2_id))
                {
                    if(!ts.triContainsVert(t_id, p_id) && genericPointInsideTriangle(ts, p_id, t_id, true))
                        g.addVertexInTriangle(t_id, p_id);
                }

                //segments inside triangle
                for(auto &seg : g.triangleSegmentsList(copl_t))
                {
                    if(genericPointInsideTriangle(ts, seg.first, t_id, false) && genericPointInsideTriangle(ts, seg.second, t_id, false) &&
                       (!ts.triContainsVert(t_id, seg.first) || !ts.triContainsVert(t_id, seg.second)))
                        g.addSegmentInTriangle(t_id, seg);
                }

            }
        }
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

void normalizeOrientations(double o[])
{
    if(o[0] < 0) o[0] = -1;
    else if(o[0] > 0) o[0] = 1;

    if(o[1] < 0) o[1] = -1;
    else if(o[1] > 0) o[1] = 1;

    if(o[2] < 0) o[2] = -1;
    else if(o[2] > 0) o[2] = 1;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

bool sameOrientation(const double &o1, const double &o2)
{
    if(o1 < 0.0 && o2 < 0.0) return true;
    if(o1 > 0.0 && o2 > 0.0) return true;
    if(o1 == 0.0 && o2 == 0.0) return true;
    return false;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

// 1 if all edges are coplanar to the triangle, -1 otherwise
bool allCoplanarEdges(const double o[])
{
    if(o[0] == 0.0 && o[1] == 0.0 && o[2] == 0.0)
        return true;
    return false;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

// if there is a coplanar edge return its id, -1 otherwise
int singleCoplanarEdge(const double o[])
{
    if(o[0] == 0.0 && o[1] == 0.0 && o[2] != 0.0) return 0;
    if(o[1] == 0.0 && o[2] == 0.0 && o[0] != 0.0) return 1;
    if(o[2] == 0.0 && o[0] == 0.0 && o[1] != 0.0) return 2;
    return -1; // false
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

// if there is a vertex in the plane and the opposite edge doesn't intersect the plane return the vtx id, -1 otherwise
int vtxInPlaneAndOppositeEdgeOnSameSide(const double o[])
{
    if(o[0] == 0.0 && o[1] == o[2] && o[1] != 0.0) return 0;
    if(o[1] == 0.0 && o[0] == o[2] && o[0] != 0.0) return 1;
    if(o[2] == 0.0 && o[0] == o[1] && o[0] != 0.0) return 2;
    return -1;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

// if there is a vertex in the plane and the opposite edge intersect the plane return the vtx id, -1 otherwise
int vtxInPlaneAndOppositeEdgeCrossPlane(const double o[])
{
    if(o[0] == 0.0 && o[1] != o[2] && o[1] != 0.0 && o[2] != 0.0) return 0;
    if(o[1] == 0.0 && o[0] != o[2] && o[0] != 0.0 && o[2] != 0.0) return 1;
    if(o[2] == 0.0 && o[0] != o[1] && o[0] != 0.0 && o[1] != 0.0) return 2;
    return -1;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

// if there is a vertex on one side and the opposite edge on the other return the relative informations, -1 otherwise
int vtxOnASideAndOppositeEdgeOnTheOther(const double o[], uint &opp_v0, uint &opp_v1)
{
    if(o[0] == 0.0 || o[1] == 0.0 || o[2] == 0.0) return -1; // one vtx on the plane

    if(o[0] == o[1] && o[1] == o[2]) return -1; // all vtx on the same side of the plane

    if(o[0] == o[1])
    {
        opp_v0 = 0;
        opp_v1 = 1;
        return 2;
    }

    if(o[0] == o[2])
    {
        opp_v0 = 0;
        opp_v1 = 2;
        return 1;
    }

    opp_v0 = 1;
    opp_v1 = 2;
    return 0;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

bool genericPointInsideTriangle(const TriangleSoup &ts, const uint &p_id, const uint &t_id, const bool &strict)
{
    const genericPoint *p = ts.vert(p_id);
    const genericPoint *tv0 = ts.triVert(t_id, 0);
    const genericPoint *tv1 = ts.triVert(t_id, 1);
    const genericPoint *tv2 = ts.triVert(t_id, 2);

    switch (ts.triPlane(t_id))
    {
        case XY:
        {
            if(strict)
                return ((genericPoint::orient2Dxy(*tv0, *tv1, *p) > 0 && genericPoint::orient2Dxy(*tv1, *tv2, *p) > 0 && genericPoint::orient2Dxy(*tv2, *tv0, *p) > 0) ||
                        (genericPoint::orient2Dxy(*tv0, *tv1, *p) < 0 && genericPoint::orient2Dxy(*tv1, *tv2, *p) < 0 && genericPoint::orient2Dxy(*tv2, *tv0, *p) < 0));
            else
                return ((genericPoint::orient2Dxy(*tv0, *tv1, *p) >= 0 && genericPoint::orient2Dxy(*tv1, *tv2, *p) >= 0 && genericPoint::orient2Dxy(*tv2, *tv0, *p) >= 0) ||
                        (genericPoint::orient2Dxy(*tv0, *tv1, *p) <= 0 && genericPoint::orient2Dxy(*tv1, *tv2, *p) <= 0 && genericPoint::orient2Dxy(*tv2, *tv0, *p) <= 0));
        }

        case YZ:
        {
            if(strict)
                return ((genericPoint::orient2Dyz(*tv0, *tv1, *p) > 0 && genericPoint::orient2Dyz(*tv1, *tv2, *p) > 0 && genericPoint::orient2Dyz(*tv2, *tv0, *p) > 0) ||
                        (genericPoint::orient2Dyz(*tv0, *tv1, *p) < 0 && genericPoint::orient2Dyz(*tv1, *tv2, *p) < 0 && genericPoint::orient2Dyz(*tv2, *tv0, *p) < 0));
            else
                return ((genericPoint::orient2Dyz(*tv0, *tv1, *p) >= 0 && genericPoint::orient2Dyz(*tv1, *tv2, *p) >= 0 && genericPoint::orient2Dyz(*tv2, *tv0, *p) >= 0) ||
                        (genericPoint::orient2Dyz(*tv0, *tv1, *p) <= 0 && genericPoint::orient2Dyz(*tv1, *tv2, *p) <= 0 && genericPoint::orient2Dyz(*tv2, *tv0, *p) <= 0));
        }

        case ZX:
        {
            if(strict)
                return ((genericPoint::orient2Dzx(*tv0, *tv1, *p) > 0 && genericPoint::orient2Dzx(*tv1, *tv2, *p) > 0 && genericPoint::orient2Dzx(*tv2, *tv0, *p) > 0) ||
                        (genericPoint::orient2Dzx(*tv0, *tv1, *p) < 0 && genericPoint::orient2Dzx(*tv1, *tv2, *p) < 0 && genericPoint::orient2Dzx(*tv2, *tv0, *p) < 0));
            else
                return ((genericPoint::orient2Dzx(*tv0, *tv1, *p) >= 0 && genericPoint::orient2Dzx(*tv1, *tv2, *p) >= 0 && genericPoint::orient2Dzx(*tv2, *tv0, *p) >= 0) ||
                        (genericPoint::orient2Dzx(*tv0, *tv1, *p) <= 0 && genericPoint::orient2Dzx(*tv1, *tv2, *p) <= 0 && genericPoint::orient2Dzx(*tv2, *tv0, *p) <= 0));
        }
    }

}
