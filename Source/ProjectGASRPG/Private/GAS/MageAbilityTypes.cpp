#include "GAS/MageAbilityTypes.h"

bool FMageGameplayEffectContext::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	/**
	 *当序列化所有对象时，构造的 所有变量都转换为0和1的字符串
	 *FArchive重载 <<左移运算符，可以用于序列化和反序列化
	 */
	uint32 RepBits = 0; //默认为无符号8位整数，用于存储8个bool值。二进制为0000 0000，每一位代表一个bool值, 因为我们用到了多个参数, 扩展到uint32

	if (Ar.IsSaving())
	{
		if (bReplicateInstigator && Instigator.IsValid())
		{
			RepBits |= 1 << 0;
			//等价于 RepBits = RepBits | (1 << 0);
			//首先计算(1 << 0)：1（0000 0001）左移0位，结果为0000 0001
			//然后进行按位或：RepBits = 0000 0000 | 0000 0001 = 0000 0001
		}
		
		if (bReplicateEffectCauser && EffectCauser.IsValid())
		{
			RepBits |= 1 << 1;
			//等价于 RepBits = RepBits | (1 << 1);
			//首先计算(1 << 1)：1（0000 0001）左移1位，结果为0000 0010
			//然后进行按位或：RepBits = 0000 0001 | 0000 0010 = 0000 0011
			//...以此类推, 每一位对应一个bool值
		}
		
		if (AbilityCDO.IsValid())
		{
			RepBits |= 1 << 2;
		}
		
		if (bReplicateSourceObject && SourceObject.IsValid())
		{
			RepBits |= 1 << 3;
		}
		
		if (Actors.Num() > 0)
		{
			RepBits |= 1 << 4;
		}
		
		if(HitResult.IsValid())
		{
			RepBits |= 1 << 5;
		}
		
		if (bHasWorldOrigin)
		{
			RepBits |= 1 << 6;
		}
		
		/** 自定义数据 */
		if (bIsCriticalHit)
		{
			RepBits |= 1 << 7;
		}

		if(bIsDebuff)
		{
			RepBits |= 1 << 8;
		}

		if(DebuffDamage > 0.f)
		{
			RepBits |= 1 << 9;
		}

		if(DebuffFrequency > 0.f)
		{
			RepBits |= 1 << 10;
		}

		if(DebuffFrequency > 0.f)
		{
			RepBits |= 1 << 11;
		}

		if(DamageTypeTag)
		{
			RepBits |= 1 << 12;
		}

		if(!DeathImpulse.IsZero())
		{
			RepBits |= 1 << 13;
		}
	}

	Ar.SerializeBits(&RepBits, 13); //序列化位数

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
	if (RepBits & (1 << 8))
	{
		Ar << bIsDebuff;
	}
	if (RepBits & (1 << 9))
	{
		Ar << DebuffDamage;
	}
	if (RepBits & (1 << 10))
	{
		Ar << DebuffFrequency;
	}
	if (RepBits & (1 << 11))
	{
		Ar << DebuffDuration;
	}
	if(RepBits & (1 << 12))
	{
		if (Ar.IsLoading())
		{
			if (!DamageTypeTag.IsValid())
			{
				DamageTypeTag = TSharedPtr<FGameplayTag>(new FGameplayTag());
			}
		}
		DamageTypeTag->NetSerialize(Ar, Map, bOutSuccess);
	}
	if(RepBits & (1 << 13))
	{
		DeathImpulse.NetSerialize(Ar, Map, bOutSuccess);
	}

	if (Ar.IsLoading())
	{
		AddInstigator(Instigator.Get(), EffectCauser.Get()); // Just to initialize InstigatorAbilitySystemComponent
	}

	bOutSuccess = true;
	return true;
}
