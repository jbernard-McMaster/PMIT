#pragma once

public interface class IProblemState
{
	property bool IsTerminal;

	virtual IProblemState^ Process();
};

