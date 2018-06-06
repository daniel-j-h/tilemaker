#ifndef _OSM_OBJECT_H
#define _OSM_OBJECT_H

#include <vector>
#include <string>
#include <sstream>
#include <map>
#include "kaguya.hpp"
#include "geomtypes.h"
#include "osm_store.h"
#include "shared_data.h"
#include "output_object.h"
#include "read_pbf.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

/**
	OSMObject - represents the object (from the .osm.pbf) currently being processed
	
	Only one instance of this class is ever used. Its main purpose is to provide a 
	consistent object for Lua to access.
	
*/

class OSMObject : public PbfReaderOutput
{ 

public:

	kaguya::State &luaState;				// Lua reference
	std::map<std::string, RTree> indices;			// Spatial indices, boost::geometry::index objects for shapefile indices
	std::vector<Geometry> &cachedGeometries;		// Cached geometries
	std::map<uint,std::string> &cachedGeometryNames;	// Cached geometry names
	OSMStore *osmStore;						// Global OSM store
	std::map< TileCoordinates, std::vector<OutputObject>, TileCoordinatesCompare > &tileIndex;

	uint64_t osmID;							// ID of OSM object
	WayID newWayID = MAX_WAY_ID;			// Decrementing new ID for relations
	bool isWay, isRelation;					// Way, node, relation?

	int32_t lon1,latp1,lon2,latp2;			// Start/end co-ordinates of OSM object
	NodeVec *nodeVec;						// node vector
	WayVec *outerWayVec, *innerWayVec;		// way vectors

	Linestring linestringCache;
	bool linestringInited;
	Polygon polygonCache;
	bool polygonInited;
	MultiPolygon multiPolygonCache;
	bool multiPolygonInited;

	const class Config &config;
	class LayerDefinition &layers;
	
	std::vector<OutputObject> outputs;			// All output objects
	std::map<std::string, std::string> currentTags;

	// ----	initialization routines

	OSMObject(const class Config &configIn, class LayerDefinition &layers, 
		kaguya::State &luaObj, std::vector<Geometry> &geomPtr, 
		std::map<uint,std::string> &namePtr, OSMStore *storePtr, 
		std::map< TileCoordinates, std::vector<OutputObject>, TileCoordinatesCompare > &tileIndex);

	// ----	Helpers provided for main routine

	// Has this object been assigned to any layers?
	bool empty();

	// ----	Set an osm element to make it accessible from Lua

	virtual void everyNode(NodeID id, LatpLon node);

	// We are now processing a node
	virtual void setNode(NodeID id, LatpLon node, const std::map<std::string, std::string> &tags);

	// We are now processing a way
	virtual void setWay(Way *way, NodeVec *nodeVecPtr, bool inRelation, const std::map<std::string, std::string> &tags);

	// We are now processing a relation
	// (note that we store relations as ways with artificial IDs, and that
	//  we use decrementing positive IDs to give a bit more space for way IDs)
	virtual void setRelation(Relation *relation, WayVec *outerWayVecPtr, WayVec *innerWayVecPtr,
		const std::map<std::string, std::string> &tags);

	// Internal: clear current cached state
	inline void reset() {
		outputs.clear();
		linestringInited = false;
		polygonInited = false;
		multiPolygonInited = false;
	}

	// Internal: set start/end co-ordinates
	inline void setLocation(int32_t a, int32_t b, int32_t c, int32_t d) {
		lon1=a; latp1=b; lon2=c; latp2=d;
	}

	// ----	Metadata queries called from Lua

	// Get the ID of the current object
	std::string Id() const;

	// Check if there's a value for a given key
	bool Holds(const std::string& key) const;

	// Get an OSM tag for a given key (or return empty string if none)
	std::string Find(const std::string& key) const;

	// ----	Spatial queries called from Lua

	// Find intersecting shapefile layer
	// TODO: multipolygon relations not supported, will always return false
	std::vector<std::string> FindIntersecting(const std::string &layerName);
	bool Intersects(const std::string &layerName);
	std::vector<uint> findIntersectingGeometries(const std::string &layerName);
	std::vector<uint> verifyIntersectResults(std::vector<IndexValue> &results, Point &p1, Point &p2);
	std::vector<std::string> namesOfGeometries(std::vector<uint> &ids);

	// Returns whether it is closed polygon
	bool IsClosed() const;

	// Scale to (kilo)meter
	double ScaleToMeter();

	double ScaleToKiloMeter();

	// Returns area
	double Area();

	// Returns length
	double Length();

	// Lazy geometries creation
	const Linestring &linestring();

	const Polygon &polygon();

	const MultiPolygon &multiPolygon();

	// ----	Requests from Lua to write this way/node to a vector tile's Layer

	// Add layer
	void Layer(const std::string &layerName, bool area);
	void LayerAsCentroid(const std::string &layerName);
	
	// Set attributes in a vector tile's Attributes table
	void Attribute(const std::string &key, const std::string &val);
	void AttributeNumeric(const std::string &key, const float val);
	void AttributeBoolean(const std::string &key, const bool val);

	// ----	vector_layers metadata entry

	void setVectorLayerMetadata(const uint_least8_t layer, const std::string &key, const uint type);
	std::string serialiseLayerJSON();
};

#endif //_OSM_OBJECT_H
