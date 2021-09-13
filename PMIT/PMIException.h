#pragma once

public ref class ProductionFailedException : Exception
{
public:
	ProductionFailedException()
	{
	}

	ProductionFailedException(String^ message) : Exception(message)
	{
	}

	ProductionFailedException(String^ message, Exception^ inner) : Exception(message, inner)
	{
	}
};

public ref class SymbolNotFoundException : Exception
{
public:
	SymbolNotFoundException()
	{
	}

	SymbolNotFoundException(String^ message) : Exception(message)
	{
	}

	SymbolNotFoundException(String^ message, Exception^ inner) : Exception(message, inner)
	{
	}
};
