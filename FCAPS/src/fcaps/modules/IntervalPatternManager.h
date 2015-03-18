#ifndef CINTERVALPATTERNMANAGER_H
#define CINTERVALPATTERNMANAGER_H

#include <fcaps/PatternDescriptor.h>
#include <fcaps/Module.h>

#include <fcaps/modules/details/JsonIntervalPattern.h>

////////////////////////////////////////////////////////////////////

const char IntervalPatternManagerModule[] = "IntervalPatternManagerModule";

////////////////////////////////////////////////////////////////////

class CIntervalPatternDescriptor : public IPatternDescriptor {
public:
	// Methods of IPatternDescriptor
	virtual TPatternType GetType() const
		{ return PT_Intervals; }
	virtual bool IsMostGeneral() const
		{return false;}
	virtual size_t Hash() const;

	JsonIntervalPattern::CPattern& Intervals()
		 {return ints;}
	const JsonIntervalPattern::CPattern& Intervals() const
		 {return ints;}
private:
	JsonIntervalPattern::CPattern ints;
};

////////////////////////////////////////////////////////////////////

class CIntervalPatternManager : public IPatternDescriptorComparator, public IModule {
public:
	CIntervalPatternManager();

	virtual TPatternType GetPatternsType() const
		{ return PT_Intervals; }

	virtual const CIntervalPatternDescriptor* LoadObject( const JSON& );
	virtual JSON SavePattern( const IPatternDescriptor* ) const;
	virtual const CIntervalPatternDescriptor* LoadPattern( const JSON& );

	virtual const CIntervalPatternDescriptor* CalculateSimilarity(
		const IPatternDescriptor* first, const IPatternDescriptor* second );
	virtual TCompareResult Compare(
		const IPatternDescriptor* first, const IPatternDescriptor* second,
		DWORD interestingResults = CR_AllResults, DWORD possibleResults = CR_AllResults | CR_Incomparable );

	virtual void FreePattern( const IPatternDescriptor * );

	virtual void Write( const IPatternDescriptor* pattern, std::ostream& dst ) const;

	// Methods of IModule
	virtual void LoadParams( const JSON& );
	virtual JSON SaveParams() const;

protected:
	CIntervalPatternDescriptor* NewPattern();
	static const CIntervalPatternDescriptor& Pattern( const IPatternDescriptor* p );

private:
	static const CModuleRegistrar<CIntervalPatternManager> registrar;

	// Number of intervals in patterns;
	DWORD intsCount;
};

#endif // CINTERVALPATTERNMANAGER_H