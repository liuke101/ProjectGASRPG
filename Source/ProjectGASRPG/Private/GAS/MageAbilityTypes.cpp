#include "GAS/MageAbilityTypes.h"

UScriptStruct* FMageGameplayEffectContext::GetScriptStruct() const
{
	return FGameplayEffectContext::GetScriptStruct();
}

bool FMageGameplayEffectContext::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	/**
	 *当序列化所有对象时，构造的 所有变量都转换为0和1的字符串
	 *FArchive重载 <<左移运算符，可以用于序列化和反序列化
	 */
	uint32 RepBits = 0; //无符号8位整数，用于存储8个bool值。二进制为0000 0000，每一位代表一个bool值

	int64 LengthBits = 0;; //记录有多少个变量需要序列化
	
	if (Ar.IsSaving())
	{
		if (bReplicateInstigator && Instigator.IsValid())
		{
			RepBits |= 1 << 0;
			//等价于 RepBits = RepBits | (1 << 0);
			//首先计算(1 << 0)：1（0000 0001）左移0位，结果为0000 0001
			//然后进行按位或：RepBits = 0000 0000 | 0000 0001 = 0000 0001
		}
		++LengthBits;
		
		if (bReplicateEffectCauser && EffectCauser.IsValid())
		{
			RepBits |= 1 << 1;
			//等价于 RepBits = RepBits | (1 << 1);
			//首先计算(1 << 1)：1（0000 0001）左移1位，结果为0000 0010
			//然后进行按位或：RepBits = 0000 0001 | 0000 0010 = 0000 0011
			//...以此类推, 每一位对应一个bool值
		}
		++LengthBits;
		
		if (AbilityCDO.IsValid())
		{
			RepBits |= 1 << 2;
		}
		++LengthBits;
		
		if (bReplicateSourceObject && SourceObject.IsValid())
		{
			RepBits |= 1 << 3;
		}
		++LengthBits;
		
		if (Actors.Num() > 0)
		{
			RepBits |= 1 << 4;
		}
		++LengthBits;
		
		if(HitResult.IsValid())
		{
			RepBits |= 1 << 5;
		}
		++LengthBits;
		
		if (bHasWorldOrigin)
		{
			RepBits |= 1 << 6;
		}
		++LengthBits;
		
		/** 自定义数据 */
		if (bIsCriticalHit)
		{
			RepBits |= 1 << 7;
		}
		++LengthBits;
	}

	Ar.SerializeBits(&RepBits, LengthBits);

	/**
	 * 获取序列化数据
	 * 
	 * 按位与，假设经过上述的处理得到了RepBits = 0111 1101
	 */
	if (RepBits & (1 << 0)) // 0111 1101 & 0000 0001 = 0000 0001, 对应的倒数第一位为1，所以从Ar中取值
	{
		Ar << Instigator;
	}
	if (RepBits & (1 << 1)) // 0111 1101 & 0000 0010 = 0000 0000, 对应的倒数第二位为0, 所以不从Ar中取值
	{
		Ar << EffectCauser;
	}
	if (RepBits & (1 << 2)) // 0111 1101 & 0000 0100 = 0000 0100, 对应的倒数第三位为1，所以从Ar中取值
	{
		Ar << AbilityCDO;
	}
	if (RepBits & (1 << 3))
	{
		Ar << SourceObject;
	}
	if (RepBits & (1 << 4))
	{
		SafeNetSerializeTArray_Default<31>(Ar, Actors);
	}
	if (RepBits & (1 << 5))
	{
		if (Ar.IsLoading())
		{
			if (!HitResult.IsValid())
			{
				HitResult = TSharedPtr<FHitResult>(new FHitResult());
			}
		}
		HitResult->NetSerialize(Ar, Map, bOutSuccess);
	}
	if (RepBits & (1 << 6))
	{
		Ar << WorldOrigin;
		bHasWorldOrigin = true;
	}
	else
	{
		bHasWorldOrigin = false;
	}
	if (RepBits & (1 << 7))
	{
		Ar << bIsCriticalHit;
	}

	if (Ar.IsLoading())
	{
		AddInstigator(Instigator.Get(), EffectCauser.Get()); // Just to initialize InstigatorAbilitySystemComponent
	}

	bOutSuccess = true;
	return true;
}

FGameplayEffectContext* FMageGameplayEffectContext::Duplicate() const
{
	return FGameplayEffectContext::Duplicate();
}
