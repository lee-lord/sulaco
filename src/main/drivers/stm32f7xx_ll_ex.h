#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f7xx.h"
#include "common/utils.h"

#define DMA_STREAM_MASK 0xFFU

__STATIC_INLINE DMA_TypeDef *LL_EX_DMA_Stream_to_DMA(DMA_Stream_TypeDef *DMAx_Streamy)
{
	// clear stream address
	return (DMA_TypeDef *) (((uint32_t) DMAx_Streamy) & ((uint32_t) ~DMA_STREAM_MASK));
}

__STATIC_INLINE uint32_t LL_EX_DMA_Stream_to_Stream(DMA_Stream_TypeDef *DMAx_Streamy)
{
	const size_t firstStreamOffset = sizeof(DMA_TypeDef);
	const size_t streamSize = sizeof(DMA_Stream_TypeDef);

	STATIC_ASSERT(DMA1_Stream0_BASE - DMA1_BASE == firstStreamOffset, DMA_TypeDef_has_padding);
	STATIC_ASSERT(DMA1_Stream1_BASE - DMA1_Stream0_BASE == streamSize, DMA_Stream_TypeDef_has_padding);

	return (((uint32_t) DMAx_Streamy & DMA_STREAM_MASK) - firstStreamOffset) / streamSize;
}

#undef DMA_STREAM_MASK

__STATIC_INLINE uint32_t LL_EX_DMA_Init(DMA_Stream_TypeDef *DMAx_Streamy, LL_DMA_InitTypeDef *DMA_InitStruct) {
	DMA_TypeDef *DMA = LL_EX_DMA_Stream_to_DMA(DMAx_Streamy);
	const uint32_t Stream = LL_EX_DMA_Stream_to_Stream(DMAx_Streamy);

	return LL_DMA_Init(DMA, Stream, DMA_InitStruct);
}

__STATIC_INLINE uint32_t LL_EX_DMA_DeInit(DMA_Stream_TypeDef *DMAx_Streamy) {
	DMA_TypeDef *DMA = LL_EX_DMA_Stream_to_DMA(DMAx_Streamy);
	const uint32_t Stream = LL_EX_DMA_Stream_to_Stream(DMAx_Streamy);

	return LL_DMA_DeInit(DMA, Stream);
}

__STATIC_INLINE void LL_EX_DMA_SetChannelSelection(DMA_Stream_TypeDef *DMAx_Streamy, uint32_t Channel)
{
	MODIFY_REG(DMAx_Streamy->CR, DMA_SxCR_CHSEL, Channel);
}

__STATIC_INLINE void LL_EX_DMA_EnableStream(DMA_Stream_TypeDef *DMAx_Streamy)
{
	SET_BIT(DMAx_Streamy->CR, DMA_SxCR_EN);
}

__STATIC_INLINE void LL_EX_DMA_DisableStream(DMA_Stream_TypeDef *DMAx_Streamy)
{
	CLEAR_BIT(DMAx_Streamy->CR, DMA_SxCR_EN);
}

__STATIC_INLINE void LL_EX_DMA_EnableIT_TC(DMA_Stream_TypeDef *DMAx_Streamy)
{
	SET_BIT(DMAx_Streamy->CR, DMA_SxCR_TCIE);
}

__STATIC_INLINE void LL_EX_DMA_SetDataLength(DMA_Stream_TypeDef* DMAx_Streamy, uint32_t NbData)
{
 	MODIFY_REG(DMAx_Streamy->NDTR, DMA_SxNDT, NbData);
}

__STATIC_INLINE void LL_EX_TIM_EnableIT(TIM_TypeDef *TIMx, uint32_t Sources)
{
	SET_BIT(TIMx->DIER, Sources);
}

__STATIC_INLINE void LL_EX_TIM_DisableIT(TIM_TypeDef *TIMx, uint32_t Sources)
{
	CLEAR_BIT(TIMx->DIER, Sources);
}

__STATIC_INLINE void LL_EX_TIM_CC_EnableNChannel(TIM_TypeDef *TIMx, uint32_t Channel)
{
	LL_TIM_CC_EnableChannel(TIMx, 4 * Channel);
}

#ifdef __cplusplus
}
#endif
