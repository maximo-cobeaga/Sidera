#include "Knowledge/AstraeonLogbookComponent.h"

void UAstraeonLogbookComponent::UpsertEntry(const FAstraeonLogbookEntry& Entry)
{
	if (Entry.EntryId.IsNone())
	{
		return;
	}

	for (FAstraeonLogbookEntry& Existing : Entries)
	{
		if (Existing.EntryId == Entry.EntryId)
		{
			Existing = Entry;
			return;
		}
	}

	Entries.Add(Entry);
}

bool UAstraeonLogbookComponent::HasEntry(FName EntryId) const
{
	return Entries.ContainsByPredicate([EntryId](const FAstraeonLogbookEntry& Entry)
	{
		return Entry.EntryId == EntryId;
	});
}
