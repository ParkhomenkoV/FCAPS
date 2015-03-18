//#include <iostream>

#include <ConsoleApplication.h>

#include <fcaps/Module.h>
#include <fcaps/ConceptBuilder.h>
#include <fcaps/Filter.h>

#include <fcaps/ModuleJSONTools.h>

#include <JSONTools.h>

#include <ListWrapper.h>

#include <rapidjson/document.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/unordered_map.hpp>

#include <iomanip>
#include <time.h>

using namespace std;
using namespace boost;

////////////////////////////////////////////////////////////////////

class CThisConsoleApplication : public CConsoleApplication, public IConceptBuilderCallback {
public:
	CThisConsoleApplication( int _argc, char** _argv ) :
		CConsoleApplication( _argc, _argv ),
		maxObjsCount( -1 )
	{
	}

	// Methods of CConsoleApplication
	virtual bool ProcessParam( const std::string& param, const std::string& value );
	virtual bool ProcessOption( const std::string& option );
	virtual bool FinalizeParams();
	virtual bool AreParamsCorrect() const;
	virtual std::string GetCmdLineDescription( const std::string& progName ) const;
	virtual int Execute();

	// Methdos of IConceptBuilderCallback
	virtual void ReportProgress( const double& p, const std::string& info ) const;

private:
	string dataPath;
	string cbPath;
	string fltrPath;
	string outBaseName;

	DWORD maxObjsCount;
	CList<DWORD> indexes;

	vector<string> objNames;

	void printException( const CException& e ) const;
	void runConceptBuilder();

	void readConceptBuilderJson( rapidjson::Document& cb ) const;
	void readDataJson( rapidjson::Document& data ) const;
	void extractObjectNames( rapidjson::Document& data );
	IConceptBuilder* createConceptBuilder( rapidjson::Document& cb ) const;

	void runFilters();
	IFilter* createFilter() const;
};

bool CThisConsoleApplication::ProcessParam( const std::string& param, const std::string& value )
{
	if ( param == "-data" ) {
		dataPath = value;
	} else if ( param == "-CB" ) {
		cbPath = value;
	} else if( param == "-fltr" ) {
		fltrPath = value;
	} else if ( param == "-out" ) {
		outBaseName = value;
	} else if ( param == "-n" ) {
		maxObjsCount = lexical_cast<DWORD>( value );
	} else if ( param == "-index" ) {
		indexes.Clear();
		vector<string> indexesStr;
		split( indexesStr, value, is_any_of( ","), token_compress_on );
		for( size_t i = 0; i < indexesStr.size(); ++i ) {
			if( indexesStr[i].empty() ) {
				continue;
			}
			const DWORD nextIndex = lexical_cast<DWORD>( indexesStr[i] );
			if( !indexes.IsEmpty() && indexes.Back() >= nextIndex ) {
				indexes.Clear();
				GetInfoStream() << "Not sorted indexes -- ignored\n";
				return true;
			}
			indexes.PushBack( nextIndex );
		}
	} else {
		return CConsoleApplication::ProcessParam( param, value );
	}
	return true;
}

bool CThisConsoleApplication::ProcessOption( const std::string& option )
{
	if ( false ) {
	} else {
		return CConsoleApplication::ProcessOption( option );
	}
	return true;
}

bool CThisConsoleApplication::FinalizeParams()
{
	return true;
}

bool CThisConsoleApplication::AreParamsCorrect() const
{
	return !dataPath.empty() && (!cbPath.empty() || !fltrPath.empty() )
		&& CConsoleApplication::AreParamsCorrect();
}

std::string CThisConsoleApplication::GetCmdLineDescription( const std::string& progName ) const
{
	return progName + " -data:{Path} [-CB:{Path}][-fltr:{PATH}] [OPTIONS]\n"
	"OPTIONS = \n"
	"\t[-out:{Path} -n:{Num} -index:{Index Set}]\n"
	">>Name of the output is {output file}-patterns\n"

	" -data -- Path to the data in JSON format (see ./JSON-Examples).\n"
	" -CB -- Path to params of Concept Builder in JSON (see JSON-Specification).\n"
	" -fltr -- Path to params of the filter applied to the lattice\n"
	" -out -- Base path of the result. Suffixes can be added.\n"
	" -n -- the number of objects to process\n"
	" -index -- add only objects with requested indexes (e.g. '-index:1,2,6')\n"
	;
}

int CThisConsoleApplication::Execute()
{
	GetInfoStream() << "Processing\n\t\"" << dataPath << "\"\n";
	if( !cbPath.empty() ) {
		GetInfoStream() << "with params from\n\t\"" << cbPath << "\"\n";
	}
	if( !fltrPath.empty() ) {
		GetInfoStream() << "with filters from\n\t\"" << fltrPath << "\"\n";
	}
	try{
		if( !cbPath.empty() ) {
			runConceptBuilder();
		} else {
			outBaseName = dataPath;
		}
		if( !fltrPath.empty() ) {
			runFilters();
		}
		GetStatusStream() << "DONE\n";
		return 0;
	} catch( CException* e ) {
		printException( *e );
		delete e;
	}

	return -1;
}

void CThisConsoleApplication::ReportProgress( const double& p, const std::string& info ) const
{
	GetStatusStream() << std::fixed << std::setprecision(3);
	GetStatusStream() << "\rProcessing " << p*100 << "%. " << info;
	GetStatusStream().flush();
}


void CThisConsoleApplication::runConceptBuilder() {
	rapidjson::Document cb;
	readConceptBuilderJson( cb );

	rapidjson::Document data;
	readDataJson( data );

	// TODO: should copy params from the context? How to copy them the PM.

	extractObjectNames( data );

	CSharedPtr<IConceptBuilder> builder ( createConceptBuilder(cb) );
	builder->SetCallback( this );

	//Iterate objects.
	time_t start = time( NULL );
	string objectJson;
	const rapidjson::Value& dataBody=data[1]["Data"];

	CStdIterator<CList<DWORD>::CConstIterator, false> index( indexes );
	DWORD objNum = 0;
	for( size_t i = 0; i < dataBody.Size(); ++i ) {
		// Select objects with good indices.
		if( !indexes.IsEmpty() ) {
			if( index.IsEnd() ) {
				break;
			}
			i = *index;
			++index;
		}
		// Cut if have processed to much.
		if( objNum >= maxObjsCount ) {
			break;
		}

		CreateStringFromJSON( dataBody[i], objectJson );
		try{
			builder->AddObject( i, objectJson );
			++objNum;
		} catch( CException* e ) {
			GetWarningStream() << "Object " << i << " has a bad description -> IGNORED\n";
			printException( *e );
			delete e;
			continue;
		}

		const time_t end = time( NULL );
		GetStatusStream() << "\rAdded " << objNum << "th object. "
			<< ". Time is " << end - start;
		GetStatusStream().flush();
	}
	GetStatusStream() << "\rAdded all objects. Processing...";
	GetStatusStream().flush();

	builder->ProcessAllObjectsAddition();

	const time_t end = time( NULL );
	GetInfoStream() << "\nTotal time is " << end - start << "\n";

	// Output base path
	if( outBaseName.empty() ) {
		outBaseName = dataPath + ".out.json";
	}

	const time_t startOutput = time( NULL );
	GetInfoStream() << "\nProducing output...";
	GetInfoStream().flush();

	builder->SaveLatticeToFile( outBaseName );

	const time_t endOutput = time( NULL );
	GetInfoStream() << "\rOutput produced " << endOutput - startOutput << " seconds\n";
}

void CThisConsoleApplication::printException( const CException& e ) const
{
	GetErrorStream() << "\nError in " << e.GetPlace()
		<< "\n" << e.GetText() << "\n\n";
}

void CThisConsoleApplication::readConceptBuilderJson( rapidjson::Document& cb ) const
{
	CJsonError jsonError;
	if( !ReadJsonFile( cbPath, cb, jsonError ) ) {
		throw new CJsonException( "ConceptBuilderJson", jsonError );
	}
	if( !cb.IsObject() ) {
		throw new CTextException( "ConceptBuilderJson", "JSON params of CB are not in an json-object" );
	}
	if( !cb.HasMember( "Params" ) ) {
		cb.AddMember( "Params", rapidjson::Value().SetObject(), cb.GetAllocator() );
	}
}
void CThisConsoleApplication::readDataJson( rapidjson::Document& data ) const
{
	CJsonError jsonError;
	if( !ReadJsonFile( dataPath, data, jsonError ) ) {
		throw new CJsonException( "readDataJson", jsonError );
	}
	if( !data.IsArray() || data.Size() < 2 ) {
		throw new CTextException( "readDataJson", "JSON data are not in an 2-sized json-array" );
	}
	if( !data[1].HasMember( "Data") ) {
		throw new CTextException( "readDataJson", "No DATA[1].Data found" );
	}
	if( !data[1]["Data"].IsArray() ) {
		throw new CTextException( "readDataJson", "DATA[1].Data should be an array" );
	}
}

void CThisConsoleApplication::extractObjectNames( rapidjson::Document& data )
{
	const rapidjson::Value& dataParams = data[0u];
	if( !dataParams.IsObject() ) {
		GetWarningStream() << "DATA[0] is not an object\n";
		return;
	}

	if( !dataParams.HasMember( "ObjNames" ) || !dataParams["ObjNames"].IsArray() ) {
		return;
	}

	const rapidjson::Value& objNamesArray = dataParams["ObjNames"];
	objNames.reserve( objNamesArray.Size() );
	for( int i = 0; i < objNamesArray.Size(); ++i) {
		const rapidjson::Value& name = objNamesArray[i];
		if( !name.IsString() ) {
			GetWarningStream() << "The " << i << "th object name is not a string -- ignored.\n";
			objNames.push_back( StdExt::to_string(i) );
			continue;
		}
		objNames.push_back( name.GetString() );
	}

	if( data[1]["Data"].Size() != objNames.size() ) {
		GetWarningStream() << "The number of objects (" << data[1]["Data"].Size() << ")"
			<< " does not correspond to the number of object names (" << objNames.size() << ").\n";
	}
}

IConceptBuilder* CThisConsoleApplication::createConceptBuilder( rapidjson::Document& cb ) const
{
	string errorText;
	auto_ptr<IConceptBuilder> builder;
	builder.reset( dynamic_cast<IConceptBuilder*>(
		CreateModuleFromJSON( cb, errorText ) ) );

	if( builder.get() == 0 ) {
		if( errorText.empty() ) {
			throw new CTextException( "execute", "PM given by params is not Pattern Manager.");
		} else {
			throw new CTextException( "execute",  errorText );
		}
	}
	builder->SetObjNames( objNames );
	return builder.release();
}

void CThisConsoleApplication::runFilters()
{
	GetInfoStream() << "\nApplying filters...";
	const time_t start = time( NULL );

	CSharedPtr<IFilter> filter ( createFilter() );
	filter->SetInputFile( outBaseName );
	filter->Process();

	const time_t end = time( NULL );
	GetInfoStream() << "\rThe filters applied in " << end - start << " seconds\n";
}

IFilter* CThisConsoleApplication::createFilter() const
{
	CJsonError jsonError;
	rapidjson::Document fltrParams;
	if( !ReadJsonFile( fltrPath, fltrParams, jsonError ) ) {
		throw new CJsonException( "ConceptBuilderJson", jsonError );
	}

	string errorText;
	auto_ptr<IFilter> filter;
	filter.reset( CreateModuleFromJSON<IFilter>( fltrParams, errorText ) );

	if( filter.get() == 0 ) {
		throw new CTextException( "execute",  errorText );
	}
	return filter.release();
}

////////////////////////////////////////////////////////////////////

int main( int argc, char *argv[] )
{
	return CThisConsoleApplication(argc, argv).Run();
}
