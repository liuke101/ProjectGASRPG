// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AssetRegistry/AssetDataTagMap.h"
#include "Async/Async.h"

struct FAssetRegistrySerializationOptions;

struct COREUOBJECT_API FAssetRegistryExportPath
{
	FAssetRegistryExportPath() = default;
	explicit FAssetRegistryExportPath(FWideStringView String);
	explicit FAssetRegistryExportPath(FAnsiStringView String);

	FName Class;
	FName Package;
	FName Object;

	FString ToString() const;
	FName ToName() const;
	void ToString(FStringBuilderBase& Out) const;

	bool IsEmpty() const { return Class.IsNone() & Package.IsNone() & Object.IsNone(); } //-V792
	explicit operator bool() const { return !IsEmpty(); }
};

bool operator==(const FAssetRegistryExportPath& A, const FAssetRegistryExportPath& B);
uint32 GetTypeHash(const FAssetRegistryExportPath& Export);

namespace FixedTagPrivate
{
	// Compact FAssetRegistryExportPath equivalent for when all FNames are numberless
	struct FNumberlessExportPath
	{
		FNameEntryId Class;
		FNameEntryId Package;
		FNameEntryId Object;

		FString ToString() const;
		FName ToName() const;
		void ToString(FStringBuilderBase& Out) const;
	};

	bool operator==(const FNumberlessExportPath& A, const FNumberlessExportPath& B);
	uint32 GetTypeHash(const FNumberlessExportPath& Export);

	enum class EValueType : uint32;

	struct FValueId
	{
		static constexpr uint32 TypeBits = 3;
		static constexpr uint32 IndexBits = 32 - TypeBits;

		EValueType 		Type : TypeBits;
		uint32 			Index : IndexBits;

		uint32 ToInt() const
		{
			return static_cast<uint32>(Type) | (Index << TypeBits);
		}

		static FValueId FromInt(uint32 Int)
		{
			return { static_cast<EValueType>((Int << IndexBits) >> IndexBits), Int >> TypeBits };
		}
	};

	struct FNumberedPair
	{
		FName Key;
		FValueId Value;
	};

	struct FNumberlessPair
	{
		FNameEntryId Key;
		FValueId Value;
	};

	// Handle to a tag value owned by a managed FStore
	struct COREUOBJECT_API FValueHandle
	{
		uint32 StoreIndex;
		FValueId Id;

		FString						AsString() const;
		FName						AsName() const;
		FAssetRegistryExportPath	AsExportPath() const;
		bool						AsText(FText& Out) const;
		bool						Equals(FStringView Str) const;
		bool						Contains(const TCHAR* Str) const;
	};

	// Handle to a tag map owned by a managed FStore
	struct COREUOBJECT_API alignas(uint64) FMapHandle
	{
		static constexpr uint32 StoreIndexBits = 14;

		uint16 IsValid : 1;
		uint16 HasNumberlessKeys : 1;
		uint16 StoreIndex : StoreIndexBits; // @see FStoreManager
		uint16 Num;
		uint32 PairBegin;

		const FValueId* FindValue(FName Key) const;

		TArrayView<const FNumberedPair>		GetNumberedView() const;
		TArrayView<const FNumberlessPair>	GetNumberlessView() const;

		// Get numbered pair at an index regardless if numberless keys are used
		FNumberedPair						At(uint32 Index) const;

		friend bool operator==(FMapHandle A, FMapHandle B);

		template<typename Func>
		void ForEachPair(Func Fn) const
		{
			if (HasNumberlessKeys != 0)
			{
				for (FNumberlessPair Pair : GetNumberlessView())
				{
					Fn(FNumberedPair{ FName::CreateFromDisplayId(Pair.Key, 0), Pair.Value });
				}
			}
			else
			{
				for (FNumberedPair Pair : GetNumberedView())
				{
					Fn(Pair);
				}
			}
		}
	};

} // end namespace FixedTagPrivate

class COREUOBJECT_API FAssetTagValueRef
{
	friend class FAssetDataTagMapSharedView;

	class FFixedTagValue
	{
		static constexpr uint64 FixedMask = uint64(1) << 63;

		uint64 Bits;

	public:
		uint64 IsFixed() const { return Bits & FixedMask; }
		uint32 GetStoreIndex() const { return static_cast<uint32>((Bits & ~FixedMask) >> 32); }
		uint32 GetValueId() const { return static_cast<uint32>(Bits); }

		FFixedTagValue() = default;
		FFixedTagValue(uint32 StoreIndex, uint32 ValueId)
			: Bits(FixedMask | (uint64(StoreIndex) << 32) | uint64(ValueId))
		{}
	};

#if PLATFORM_32BITS
	class FStringPointer
	{
		uint64 Ptr;

	public:
		FStringPointer() = default;
		explicit FStringPointer(const FString* InPtr) : Ptr(reinterpret_cast<uint64>(InPtr)) {}
		FStringPointer& operator=(const FString* InPtr) { Ptr = reinterpret_cast<uint64>(InPtr); return *this; }

		const FString* operator->() const { return reinterpret_cast<const FString*>(Ptr); }
		operator const FString* () const { return reinterpret_cast<const FString*>(Ptr); }
	};
#else
	using FStringPointer = const FString*;
#endif

	union
	{
		FStringPointer Loose;
		FFixedTagValue Fixed;
		uint64 Bits = 0;
	};

	uint64 IsFixed() const { return Fixed.IsFixed(); }
	FixedTagPrivate::FValueHandle AsFixed() const;
	const FString& AsLoose() const;

public:
	FAssetTagValueRef() = default;
	FAssetTagValueRef(const FAssetTagValueRef&) = default;
	FAssetTagValueRef(FAssetTagValueRef&&) = default;
	explicit FAssetTagValueRef(const FString* Str) : Loose(Str) {}
	FAssetTagValueRef(uint32 StoreIndex, FixedTagPrivate::FValueId ValueId) : Fixed(StoreIndex, ValueId.ToInt()) {}

	FAssetTagValueRef& operator=(const FAssetTagValueRef&) = default;
	FAssetTagValueRef& operator=(FAssetTagValueRef&&) = default;

	bool						IsSet() const { return Bits != 0; }

	FString						AsString() const;
	FName						AsName() const;
	FAssetRegistryExportPath	AsExportPath() const;
	FText						AsText() const;
	bool						TryGetAsText(FText& Out) const; // @return false if value isn't a localized string

	FString						GetValue() const { return AsString(); }

	// Get FTexts as unlocalized complex strings. For internal use only, to make new FAssetDataTagMapSharedView.
	FString						ToLoose() const;

	bool						Equals(FStringView Str) const;

	UE_DEPRECATED(4.27, "Use AsString(), AsName(), AsExportPath() or AsText() instead. ")
		operator FString () const { return AsString(); }
};

inline bool operator==(FAssetTagValueRef A, FStringView B) { return  A.Equals(B); }
inline bool operator!=(FAssetTagValueRef A, FStringView B) { return !A.Equals(B); }
inline bool operator==(FStringView A, FAssetTagValueRef B) { return  B.Equals(A); }
inline bool operator!=(FStringView A, FAssetTagValueRef B) { return !B.Equals(A); }

// These overloads can be removed when the deprecated implicit operator FString is removed
inline bool operator==(FAssetTagValueRef A, const FString& B) { return  A.Equals(B); }
inline bool operator!=(FAssetTagValueRef A, const FString& B) { return !A.Equals(B); }
inline bool operator==(const FString& A, FAssetTagValueRef B) { return  B.Equals(A); }
inline bool operator!=(const FString& A, FAssetTagValueRef B) { return !B.Equals(A); }


namespace FixedTagPrivate
{
	/// Stores a fixed set of values and all the key-values maps used for lookup
	struct FStore
	{
		// Pairs for all unsorted maps that uses this store 
		TArrayView<FNumberedPair> Pairs;
		TArrayView<FNumberlessPair> NumberlessPairs;

		// Values for all maps in this store
		TArrayView<uint32> AnsiStringOffsets;
		TArrayView<ANSICHAR> AnsiStrings;
		TArrayView<uint32> WideStringOffsets;
		TArrayView<WIDECHAR> WideStrings;
		TArrayView<FNameEntryId> NumberlessNames;
		TArrayView<FName> Names;
		TArrayView<FNumberlessExportPath> NumberlessExportPaths;
		TArrayView<FAssetRegistryExportPath> ExportPaths;
		TArrayView<FText> Texts;

		const uint32 Index;
		void* Data = nullptr;

		void AddRef() const { RefCount.Increment(); }
		COREUOBJECT_API void Release() const;
		
		const ANSICHAR* GetAnsiString(uint32 Idx) const { return &AnsiStrings[AnsiStringOffsets[Idx]]; }
		const WIDECHAR* GetWideString(uint32 Idx) const { return &WideStrings[WideStringOffsets[Idx]]; }

	private:
		friend class FStoreManager;
		explicit FStore(uint32 InIndex) : Index(InIndex) {}
		~FStore();

		mutable FThreadSafeCounter RefCount;
	};

	struct FOptions
	{
		TSet<FName> StoreAsName;
		TSet<FName> StoreAsPath;
	};

	// Incomplete handle to a map in an unspecified FStore.
	// Used for serialization where the store index is implicit.
	struct COREUOBJECT_API alignas(uint64) FPartialMapHandle
	{
		bool bHasNumberlessKeys = false;
		uint16 Num = 0;
		uint32 PairBegin = 0;

		FMapHandle MakeFullHandle(uint32 StoreIndex) const;
		uint64 ToInt() const;
		static FPartialMapHandle FromInt(uint64 Int);
	};

	// Note: Can be changed to a single allocation and array views to improve cooker performance
	struct FStoreData
	{
		TArray<FNumberedPair> Pairs;
		TArray<FNumberlessPair> NumberlessPairs;

		TArray<uint32> AnsiStringOffsets;
		TArray<ANSICHAR> AnsiStrings;
		TArray<uint32> WideStringOffsets;
		TArray<WIDECHAR> WideStrings;
		TArray<FNameEntryId> NumberlessNames;
		TArray<FName> Names;
		TArray<FNumberlessExportPath> NumberlessExportPaths;
		TArray<FAssetRegistryExportPath> ExportPaths;
		TArray<FText> Texts;
	};

	uint32 HashCaseSensitive(const TCHAR* Str, int32 Len);
	uint32 HashCombineQuick(uint32 A, uint32 B);
	uint32 HashCombineQuick(uint32 A, uint32 B, uint32 C);

	// Helper class for saving or constructing an FStore
	class FStoreBuilder
	{
	public:
		explicit FStoreBuilder(const FOptions& InOptions) : Options(InOptions) {}
		explicit FStoreBuilder(FOptions&& InOptions) : Options(MoveTemp(InOptions)) {}

		COREUOBJECT_API FPartialMapHandle AddTagMap(const FAssetDataTagMapSharedView& Map);

		// Call once after all tag maps have been added
		COREUOBJECT_API FStoreData Finalize();

	private:

		template <typename ValueType>
		struct FCaseSensitiveFuncs : BaseKeyFuncs<ValueType, FString, /*bInAllowDuplicateKeys*/ false>
		{
			template<typename KeyType>
			static const KeyType& GetSetKey(const TPair<KeyType, ValueType>& Element)
			{
				return Element.Key;
			}

			static bool Matches(const FString& A, const FString& B)
			{
				return A.Equals(B, ESearchCase::CaseSensitive);
			}
			static uint32 GetKeyHash(const FString& Key)
			{
				return HashCaseSensitive(&Key[0], Key.Len());
			}

			static bool Matches(FNameEntryId A, FNameEntryId B)
			{
				return A == B;
			}
			static uint32 GetKeyHash(FNameEntryId Key)
			{
				return GetTypeHash(Key);
			}

			static bool Matches(FName A, FName B)
			{
				return (A.GetDisplayIndex() == B.GetDisplayIndex()) & (A.GetNumber() == B.GetNumber());
			}
			static uint32 GetKeyHash(FName Key)
			{
				return HashCombineQuick(GetTypeHash(Key.GetDisplayIndex()), Key.GetNumber());
			}

			template<class ExportPathType>
			static bool Matches(const ExportPathType& A, const ExportPathType& B)
			{
				return Matches(A.Class, B.Class) & Matches(A.Package, B.Package) & Matches(A.Object, B.Object); //-V792
			}

			template<class ExportPathType>
			static uint32 GetKeyHash(const ExportPathType& Key)
			{
				return HashCombineQuick(GetKeyHash(Key.Class), GetKeyHash(Key.Package), GetKeyHash(Key.Object));
			}
		};

		struct FStringIndexer
		{
			uint32 NumCharacters = 0;
			TMap<FString, uint32, FDefaultSetAllocator, FCaseSensitiveFuncs<uint32>> StringIndices;
			TArray<uint32> Offsets;

			uint32 Index(FString&& String);

			TArray<ANSICHAR> FlattenAsAnsi() const;
			TArray<WIDECHAR> FlattenAsWide() const;
		};

		const FOptions Options;
		FStringIndexer AnsiStrings;
		FStringIndexer WideStrings;
		TMap<FNameEntryId, uint32> NumberlessNameIndices;
		TMap<FName, uint32, FDefaultSetAllocator, FCaseSensitiveFuncs<uint32>> NameIndices;
		TMap<FNumberlessExportPath, uint32, FDefaultSetAllocator, FCaseSensitiveFuncs<uint32>> NumberlessExportPathIndices;
		TMap<FAssetRegistryExportPath, uint32, FDefaultSetAllocator, FCaseSensitiveFuncs<uint32>> ExportPathIndices;
		TMap<FString, uint32, FDefaultSetAllocator, FCaseSensitiveFuncs<uint32>> TextIndices;

		TArray<FNumberedPair> NumberedPairs;
		TArray<FNumberedPair> NumberlessPairs; // Stored as numbered for convenience

		bool bFinalized = false;

		FValueId IndexValue(FName Key, FAssetTagValueRef Value);
	};

	enum class ELoadOrder { Member, TextFirst };

	COREUOBJECT_API void SaveStore(const FStoreData& Store, FArchive& Ar);
	COREUOBJECT_API TRefCountPtr<const FStore> LoadStore(FArchive& Ar);

	/// Loads tag store with async creation of expensive tag values
	///
	/// Caller should:
	/// * Call ReadInitialDataAndKickLoad()
	/// * Call LoadFinalData()
	/// * Wait for future before resolving stored tag values
	class FAsyncStoreLoader
	{
	public:
		COREUOBJECT_API FAsyncStoreLoader();

		/// 1) Read initial data and kick expensive tag value creation task
		///
		/// Won't load FNames to allow concurrent name batch loading
		/// 
		/// @return handle to step 3
		COREUOBJECT_API TFuture<void> ReadInitialDataAndKickLoad(FArchive& Ar, uint32 MaxWorkerTasks);

		/// 2) Read remaining data, including FNames
		///
		/// @return indexed store, usable for FPartialMapHandle::MakeFullHandle()
		COREUOBJECT_API TRefCountPtr<const FStore> LoadFinalData(FArchive& Ar);

	private:
		TRefCountPtr<FStore> Store;
		TOptional<ELoadOrder> Order;
	};

} // end namespace FixedTagPrivate