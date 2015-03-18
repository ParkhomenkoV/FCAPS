#include <fcaps/modules/SofyaConceptBuilder.h>

#include <fcaps/storages/CachedIntentStorage.h>

#include <fcaps/ModuleJSONTools.h>

#include <JSONTools.h>

#include <fcaps/tools/FindConceptOrder.h>

#include <rapidjson/document.h>

using namespace std;

////////////////////////////////////////////////////////////////////
CModuleRegistrar<CSofyaConceptBuilder> CSofyaConceptBuilder::registrar(
	ConceptBuilderModuleType, SofyaConceptBuilder );

CSofyaConceptBuilder::CSofyaConceptBuilder() :
	callback(0),
	thld( 0 ),
	mpn( 1000 ),
	shouldFindPartialOrder(false)
{
	storage.Reserve( mpn );
}

void CSofyaConceptBuilder::LoadParams( const JSON& json )
{
	CJsonError error;
	rapidjson::Document params;
	if( !ReadJsonString( json, params, error ) ) {
		throw new CJsonException( "CSofyaConceptBuilder::LoadParams", error );
	}
	assert( string( params["Type"].GetString() ) == ConceptBuilderModuleType );
	assert( string( params["Name"].GetString() ) == SofyaConceptBuilder );
	if( !(params.HasMember( "Params" ) && params["Params"].IsObject()) ) {
		error.Data = json;
		error.Error = "Params is not found. Necessary for ProjectionChain";
		throw new CJsonException("CSofyaConceptBuilder::LoadParams", error);
	}

	const rapidjson::Value& p = params["Params"];
	if( p.HasMember( "DefualtThld" )) {
		const rapidjson::Value& thldJson = params["Params"]["DefualtThld"];
		if( thldJson.IsNumber() ) {
			thld = thldJson.GetDouble();
		}
	}
	if( p.HasMember( "MaxPatternNumber" ) ) {
		const rapidjson::Value& mpnJson = params["Params"]["MaxPatternNumber"];
		if( mpnJson.IsUint() ) {
			mpn = mpnJson.GetUint();
		}
	}
	if( p.HasMember( "FindPartialOrder" ) ) {
		const rapidjson::Value& poJson = params["Params"]["FindPartialOrder"];
		if( poJson.IsBool() ) {
			shouldFindPartialOrder = poJson.GetBool();
		}
	}

	if( !p.HasMember("ProjectionChain") ) {
		error.Data = json;
		error.Error = "Params is not found. Necessary for ProjectionChain";
		throw new CJsonException("CSofyaConceptBuilder::LoadParams", error);
	}

	const rapidjson::Value& pc = params["Params"]["ProjectionChain"];
	string errorText;
	pChain.reset( CreateModuleFromJSON<IProjectionChain>(pc,errorText) );
	if( pChain == 0 ) {
		throw new CJsonException( "CSofyaConceptBuilder::LoadParams", CJsonError( json, errorText ) );
	}
	storage.Initialize(CHasher(pChain));
	storage.Reserve( mpn );
}
JSON CSofyaConceptBuilder::SaveParams() const
{
	rapidjson::Document params;
	rapidjson::MemoryPoolAllocator<>& alloc = params.GetAllocator();
	params.SetObject()
		.AddMember( "Type", ConceptBuilderModuleType, alloc )
		.AddMember( "Name", SofyaConceptBuilder, alloc )
		.AddMember( "Params", rapidjson::Value().SetObject()
			.AddMember( "Thld", rapidjson::Value().SetDouble( thld ), alloc )
			.AddMember( "MaxPatternNumber", rapidjson::Value().SetUint( mpn ), alloc ),
		alloc );
	IModule* m = dynamic_cast<IModule*>(pChain.get());
	assert( m!=0);
	if( m != 0 ) {
		JSON pChainParams = m->SaveParams();
		rapidjson::Document pChainParamsDoc;
		CJsonError error;
		const bool rslt = ReadJsonString( pChainParams, pChainParamsDoc, error );
		assert(rslt);
		params["Params"].AddMember("ProjectionChain", pChainParamsDoc.Move(), alloc );
	}

	JSON result;
	CreateStringFromJSON( params, result );
	return result;
}

const std::vector<std::string>& CSofyaConceptBuilder::GetObjNames() const
{
	assert(pChain != 0);
	return pChain->GetObjNames();
}
void CSofyaConceptBuilder::SetObjNames( const std::vector<std::string>& names )
{
	assert(pChain != 0);
	pChain->SetObjNames( names );
}

void CSofyaConceptBuilder::AddObject( DWORD objectNum, const JSON& intent )
{
	assert(pChain != 0);
	pChain->AddObject(objectNum, intent);
}

void CSofyaConceptBuilder::ProcessAllObjectsAddition()
{
	assert( pChain != 0 );
	pChain->UpdateInterestThreshold( thld );
	IProjectionChain::CPatternList newPatterns;
	pChain->ComputeZeroProjection( newPatterns );
	addNewPatterns( newPatterns );

	while( pChain->NextProjection() ) {
		newPatterns.Clear();
		CCachedPatternStorage<CHasher>::CIteratorWrapper itr = storage.Iterator();
		while( !itr.IsEnd() ) {
			CCachedPatternStorage<CHasher>::CIteratorWrapper currItr = itr;
			++itr;

			pChain->Preimages( *currItr, newPatterns );
			if( pChain->GetPatternInterest( *currItr ) < thld ) {
				// Normally the iterator should not be invalidated here
				// Sure?
				storage.RemovePattern(*currItr);
			}
		}
		addNewPatterns( newPatterns );
		adjustThreshold();
		reportProgress();
	}
	reportProgress();
}

class CSofyaConceptBuilder::CConceptsForOrder {
public:
	CConceptsForOrder( const IProjectionChain& _chain, const vector<CPatternMeasurePair>& _concepts ) :
		cmp( _chain ), concepts( _concepts ) {}

	// Methods of TConcepts requiring by CFindConceptOrder
	DWORD Size() const
		{ return concepts.size(); }
	bool IsTopologicallyLess( DWORD c1, DWORD c2 ) const
	{
		assert( c1 < concepts.size() );
		assert( c2 < concepts.size() );
		return cmp.IsTopoSmaller(concepts[c1].first, concepts[c2].first);
	}
	bool IsLess( DWORD c1, DWORD c2 ) const
	{
		assert( c1 < concepts.size() );
		assert( c2 < concepts.size() );
		const bool res =
			cmp.IsSmaller( concepts[c1].first, concepts[c2].first);
		return res;
	}
private:
	const IProjectionChain& cmp;
	const vector<CPatternMeasurePair>& concepts;
};
void CSofyaConceptBuilder::SaveLatticeToFile( const std::string& path )
{
	vector<CPatternMeasurePair> concepts;
	concepts.reserve(storage.Size() );
	CCachedPatternStorage<CHasher>::CIteratorWrapper pItr = storage.Iterator();
	for( ; !pItr.IsEnd(); ++pItr ) {
		const IPatternDescriptor* p = *pItr;
		concepts.push_back( CPatternMeasurePair( p, pChain->GetPatternInterest(p) ) );
	}
	sort( concepts.begin(), concepts.end(), compareCachedPatterns );
	assert( concepts.size() == 0 || concepts.back().second >= thld );

	CConceptsForOrder conceptsForOrder( *pChain, concepts );
	CFindConceptOrder<CConceptsForOrder> conceptOrderFinder( conceptsForOrder );
	if( shouldFindPartialOrder ) {
		conceptOrderFinder.Compute();
	}

	saveToFile( concepts, conceptOrderFinder, path );
}

// Adds new patterns to
void CSofyaConceptBuilder::addNewPatterns( const IProjectionChain::CPatternList& newPatterns )
{
	CStdIterator<IProjectionChain::CPatternList::CConstIterator, false> itr(newPatterns);
	DWORD i = 0;
	for( ; !itr.IsEnd(); ++itr, ++i ) {
		const IPatternDescriptor* p = storage.AddPattern( *itr );
		assert( pChain->GetPatternInterest(p) - thld >= -0.00001*thld );
	}
}

////////////////////////////////////////////////////////////////////
// CSofyaConceptBuilder::adjustThreshold stuff

inline bool CSofyaConceptBuilder::compareCachedPatterns(
	const CPatternMeasurePair& p1, const CPatternMeasurePair& p2)
{
	return p1.second > p2.second;
}

// Adjusts threshold in order to be in memory
void CSofyaConceptBuilder::adjustThreshold()
{
	if( storage.Size() <= mpn ) {
		return;
	}

	vector<CPatternMeasurePair> measures;
	measures.reserve(storage.Size() );
	CCachedPatternStorage<CHasher>::CIteratorWrapper pItr = storage.Iterator();
	for( ; !pItr.IsEnd(); ++pItr ) {
		measures.push_back( CPatternMeasurePair( *pItr, pChain->GetPatternInterest(*pItr) ) );
	}
	sort( measures.begin(), measures.end(), compareCachedPatterns );
	assert( measures.front().second > thld );

	// Finding the final threshold.
	// Too slow?
	// What if Zero?s
	DWORD finalSize = mpn;
	while( abs((measures[finalSize-1].second - measures[mpn].second)/measures[mpn].second) < 0.00001 ) {
		--finalSize;
	}
	thld = (measures[finalSize-1].second + measures[finalSize].second)/2;

	for( int i = finalSize; i < measures.size(); ++i ) {
		storage.RemovePattern(measures[i].first);
	}
	pChain->UpdateInterestThreshold( thld );
}

void CSofyaConceptBuilder::reportProgress() const
{
	if( callback == 0 ) {
		return;
	}
	callback->ReportProgress( pChain->GetProgress(),
		"Concepts: " + StdExt::to_string(storage.Size())
		+ " Thld: " + StdExt::to_string(thld) );
}

void CSofyaConceptBuilder::saveToFile(
	const std::vector<CPatternMeasurePair>& concepts,
	const CFindConceptOrder<CConceptsForOrder>& order,
	const std::string& path )
{
	CDestStream dst(path);
	CList<DWORD> tops;
	vector<bool> bottoms;
	bottoms.resize( concepts.size(), true );
	DWORD arcsCount = 0;
	for( DWORD i = 0; i < concepts.size(); ++i ) {
		const CList<DWORD>& parents = order.GetParents( i );
		arcsCount += parents.Size();
		if( parents.Size() == 0 ) {
			tops.PushBack( i );
		}
		CStdIterator<CList<DWORD>::CConstIterator,false> p( parents );
		for( ; !p.IsEnd(); ++p ) {
			bottoms[*p] = false;
		}
	}

	bool isFirst = true;

	dst << "[";

	// Params of the lattice
	dst << "{";
		dst << "\"NodesCount\":" << concepts.size() << ",";
		dst << "\"ArcsCount\":" << arcsCount << ",";

		dst << "\"Top\":[";
		CStdIterator<CList<DWORD>::CConstIterator, false> top(tops);
		for( ;!top.IsEnd(); ++top ) {
			if(!isFirst) {
				dst << ",";
			}
			isFirst = false;
			dst << *top;
		}
		dst << "],";

		dst << "\"Bottom\":[";
		isFirst=true;
		for( DWORD i = 0; i < bottoms.size(); ++i ) {
			if(!bottoms[i]) {
				continue;
			}
			if(!isFirst) {
				dst << ",";
			}
			isFirst=false;
			dst << i;
		}
		dst << "],";
		dst << "\"Params\":" << SaveParams();
	dst << "},";

	// Nodes of the lattice
	dst << "{";
		dst << "\"Nodes\":[\n";
		isFirst=true;
		for( DWORD i = 0; i < concepts.size(); ++i ) {
			if(!isFirst){
				dst << ",\n";
			}
			isFirst=false;
			printConceptToJson( concepts[i], dst );
		}
		dst << "\n]";
	dst << "},";

	// Arcs of the poset
	dst << "{";
		dst << "\"Arcs\":[";
		isFirst = true;
		if( shouldFindPartialOrder ) {
			for( DWORD i = 0; i < concepts.size(); ++i ) {
				CStdIterator<CList<DWORD>::CConstIterator,false> p( order.GetParents( i ) );
				for( ;!p.IsEnd(); ++p ) {
					if(!isFirst) {
						dst << ",";
					}
					isFirst=false;
					dst << "{\"S\":" << *p << ",\"D\":" << i << "}";
				}
			}
		}
		dst << "]";
	dst << "}";

	dst << "]\n";
}

void CSofyaConceptBuilder::printConceptToJson( const CPatternMeasurePair& c, std::ostream& dst )
{
dst << "{";

	dst << "\"Ext\":" << pChain->SaveExtent( c.first ) << ",";
	dst << "\"Int\":" << pChain->SaveIntent( c.first ) << ",";

	dst << "\"Interest\":" << c.second;

dst << "}";
}
