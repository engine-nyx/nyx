#include <future/stdbit.h>
#include <nyx/types.h>
#include <nyx/utils.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

constexpr size_t LUT_BISHOP_SIZE = 5248;
constexpr size_t LUT_ROOK_SIZE = 102400;

bitboard lut_bishop_mask    [NUM_SQUARES];
bitboard lut_bishop_offset  [NUM_SQUARES];
bitboard lut_bishop_attacks [LUT_BISHOP_SIZE];

bitboard lut_rook_mask      [NUM_SQUARES];
bitboard lut_rook_offset    [NUM_SQUARES];
bitboard lut_rook_attacks   [LUT_ROOK_SIZE];

bitboard lut_knight_attacks [NUM_SQUARES];
bitboard lut_king_attacks   [NUM_SQUARES];

static constexpr bitboard file_attack = 0x0001010101010100ull;
static constexpr bitboard rank_attack = 0x000000000000007Eull;
static constexpr bitboard not_a_file = ~0x0101010101010101ull;
static constexpr bitboard not_h_file = ~0x8080808080808080ull;
static constexpr bitboard not_1_rank = ~0x00000000000000FFull;
static constexpr bitboard not_8_rank = ~0xFF00000000000000ull;
static constexpr bitboard no_edges = not_a_file & not_h_file & not_1_rank & not_8_rank;

static inline bitboard no(bitboard b) { return (b << 8); }
static inline bitboard so(bitboard b) { return (b >> 8); }
static inline bitboard ea(bitboard b) { return (b << 1) & not_a_file; }
static inline bitboard we(bitboard b) { return (b >> 1) & not_h_file; }
static inline bitboard noea(bitboard b) { return no(ea(b)); }
static inline bitboard soea(bitboard b) { return so(ea(b)); }
static inline bitboard nowe(bitboard b) { return no(we(b)); }
static inline bitboard sowe(bitboard b) { return so(we(b)); }
static inline bitboard nonoea(bitboard b) { return no(noea(b)); }
static inline bitboard noeaea(bitboard b) { return ea(noea(b)); }
static inline bitboard soeaea(bitboard b) { return ea(soea(b)); }
static inline bitboard sosoea(bitboard b) { return so(soea(b)); }
static inline bitboard nonowe(bitboard b) { return no(nowe(b)); }
static inline bitboard nowewe(bitboard b) { return we(nowe(b)); }
static inline bitboard sowewe(bitboard b) { return we(sowe(b)); }
static inline bitboard sosowe(bitboard b) { return so(sowe(b)); }

static bitboard
calc_bishop_attacks(bitboard occ, square sq)
{
	bitboard attacks, bishop;

	attacks = 0;

	for (bishop = sqbb(sq); bishop; bishop &= ~occ) attacks |= (bishop = noea(bishop));
	for (bishop = sqbb(sq); bishop; bishop &= ~occ) attacks |= (bishop = soea(bishop));
	for (bishop = sqbb(sq); bishop; bishop &= ~occ) attacks |= (bishop = nowe(bishop));
	for (bishop = sqbb(sq); bishop; bishop &= ~occ) attacks |= (bishop = sowe(bishop));

	return attacks;
}

static void
lut_fill_bishop_attacks(void)
{
	size_t offset;
	square sq;
	unsigned num_entries;
	bitboard board;

	offset = 0;

	for (sq = A1; sq < NUM_SQUARES; ++sq)
	{
		lut_bishop_offset[sq] = offset;

		// generate mask
		for (board = sqbb(sq); board; board = noea(board)) lut_bishop_mask[sq] |= board;
		for (board = sqbb(sq); board; board = soea(board)) lut_bishop_mask[sq] |= board;
		for (board = sqbb(sq); board; board = nowe(board)) lut_bishop_mask[sq] |= board;
		for (board = sqbb(sq); board; board = sowe(board)) lut_bishop_mask[sq] |= board;
		lut_bishop_mask[sq] &= ~sqbb(sq);
		lut_bishop_mask[sq] &= no_edges;

		num_entries = stdc_count_ones(lut_bishop_mask[sq]);
		for (board = 0; board < (1ull << num_entries); ++board)
		{
			lut_bishop_attacks[offset++] = calc_bishop_attacks(stdc_bit_expand(board, lut_bishop_mask[sq]), sq);
		}
	}
}

static bitboard
calc_rook_attacks(bitboard occ, square sq)
{
	bitboard attacks, rook;

	attacks = 0;

	for (rook = sqbb(sq); rook; rook &= ~occ) attacks |= (rook = no(rook));
	for (rook = sqbb(sq); rook; rook &= ~occ) attacks |= (rook = so(rook));
	for (rook = sqbb(sq); rook; rook &= ~occ) attacks |= (rook = ea(rook));
	for (rook = sqbb(sq); rook; rook &= ~occ) attacks |= (rook = we(rook));

	return attacks;
}

static void
lut_fill_rook_attacks(void)
{
	size_t offset;
	square sq;
	unsigned file, rank, num_entries;
	bitboard board;

	offset = 0;

	for (sq = A1; sq < NUM_SQUARES; ++sq)
	{
		lut_rook_offset[sq] = offset;

		file = sq &  BITMASK(3);
		rank = (sq >> 3) << 3;

		// generate mask
		lut_rook_mask[sq] |= file_attack << file;
		lut_rook_mask[sq] |= rank_attack << rank;
		lut_rook_mask[sq] &= ~sqbb(sq);

		num_entries = stdc_count_ones(lut_rook_mask[sq]);
		for (board = 0; board < (1ull << num_entries); ++board)
		{
			lut_rook_attacks[offset++] = calc_rook_attacks(stdc_bit_expand(board, lut_rook_mask[sq]), sq);
		}
	}
}

static void
lut_fill_knight_attacks(void)
{
	square sq;
	bitboard knight;

	for (sq = A1; sq < NUM_SQUARES; ++sq)
	{
		knight = sqbb(sq);

		lut_knight_attacks[sq] |= nonoea(knight);
		lut_knight_attacks[sq] |= noeaea(knight);
		lut_knight_attacks[sq] |= soeaea(knight);
		lut_knight_attacks[sq] |= sosoea(knight);
		lut_knight_attacks[sq] |= nonowe(knight);
		lut_knight_attacks[sq] |= nowewe(knight);
		lut_knight_attacks[sq] |= sowewe(knight);
		lut_knight_attacks[sq] |= sosowe(knight);
	}
}

static void
lut_fill_king_attacks(void)
{
	square sq;
	bitboard king;

	for (sq = A1; sq < NUM_SQUARES; ++sq)
	{
		king = sqbb(sq);

		lut_king_attacks[sq] |= no(king);
		lut_king_attacks[sq] |= so(king);
		lut_king_attacks[sq] |= ea(king);
		lut_king_attacks[sq] |= we(king);
		lut_king_attacks[sq] |= noea(king);
		lut_king_attacks[sq] |= soea(king);
		lut_king_attacks[sq] |= nowe(king);
		lut_king_attacks[sq] |= sowe(king);
	}
}

static void
lut_init(void)
{
	lut_fill_bishop_attacks();
	lut_fill_rook_attacks();
	lut_fill_knight_attacks();
	lut_fill_king_attacks();
}

static void
lut_dump(bitboard *lut, size_t size, const char *fname)
{
	FILE *file;

	if (!(file = fopen(fname, "wb")))
	{
		perror("Error opening file");
		return;
	}

	if (fwrite(lut, sizeof(bitboard), size, file) != size)
	{
		fprintf(stderr, "Error writing to %s\n", fname);
	}

	fclose(file);
}

int
main(int argc, char **argv)
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <LUT dir>", argv[0]);
		return EXIT_FAILURE;
	}

	char fname_buf[strlen(argv[1]) + 20];

	lut_init();

	strcpy(fname_buf, argv[1]), strcat(fname_buf, "/bishop_mask.bin");
	lut_dump(lut_bishop_mask, NUM_SQUARES, fname_buf);

	strcpy(fname_buf, argv[1]), strcat(fname_buf, "/bishop_offset.bin");
	lut_dump(lut_bishop_offset, NUM_SQUARES, fname_buf);

	strcpy(fname_buf, argv[1]), strcat(fname_buf, "/bishop_attacks.bin");
	lut_dump(lut_bishop_attacks, LUT_BISHOP_SIZE, fname_buf);

	strcpy(fname_buf, argv[1]), strcat(fname_buf, "/rook_mask.bin");
	lut_dump(lut_rook_mask, NUM_SQUARES, fname_buf);

	strcpy(fname_buf, argv[1]), strcat(fname_buf, "/rook_offset.bin");
	lut_dump(lut_rook_offset, NUM_SQUARES, fname_buf);

	strcpy(fname_buf, argv[1]), strcat(fname_buf, "/rook_attacks.bin");
	lut_dump(lut_rook_attacks, LUT_ROOK_SIZE, fname_buf);

	strcpy(fname_buf, argv[1]), strcat(fname_buf, "/knight_attacks.bin");
	lut_dump(lut_knight_attacks, NUM_SQUARES, fname_buf);

	strcpy(fname_buf, argv[1]), strcat(fname_buf, "/king_attacks.bin");
	lut_dump(lut_king_attacks, NUM_SQUARES, fname_buf);

	return EXIT_SUCCESS;
}
