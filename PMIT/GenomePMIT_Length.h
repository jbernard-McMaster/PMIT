#pragma once

using namespace System;

public ref class GenomePMIT_Length :
	public GenomeInt
{
public:
	GenomePMIT_Length(Int32 NumGenes, array<Int32>^ Min, array<Int32>^ Max);
	virtual ~GenomePMIT_Length();

	virtual void Mutate(Int32 Index) override;
};

