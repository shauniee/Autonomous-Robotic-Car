#ifndef CODE39_VECTORS_H
#define CODE39_VECTORS_H
/* Standard wide-element positions, independently checked against ZXing's
   published Code39Reader CHARACTER_ENCODINGS (A-Z) on 2026-10-14.
   Data patterns are not a claim that the issued course uses Code 39. */
static const unsigned code39_wide[26][3] = {
    {0, 5, 8}, /* A */
    {2, 5, 8}, /* B */
    {0, 2, 5}, /* C */
    {4, 5, 8}, /* D */
    {0, 4, 5}, /* E */
    {2, 4, 5}, /* F */
    {5, 6, 8}, /* G */
    {0, 5, 6}, /* H */
    {2, 5, 6}, /* I */
    {4, 5, 6}, /* J */
    {0, 7, 8}, /* K */
    {2, 7, 8}, /* L */
    {0, 2, 7}, /* M */
    {4, 7, 8}, /* N */
    {0, 4, 7}, /* O */
    {2, 4, 7}, /* P */
    {6, 7, 8}, /* Q */
    {0, 6, 7}, /* R */
    {2, 6, 7}, /* S */
    {4, 6, 7}, /* T */
    {0, 1, 8}, /* U */
    {1, 2, 8}, /* V */
    {0, 1, 2}, /* W */
    {1, 4, 8}, /* X */
    {0, 1, 4}, /* Y */
    {1, 2, 4}, /* Z */
};
#endif
