
#include "inv-index.h"

Tokenizer *
tokenizer_new (const gchar *str)
{
    return tokenizer_new2(str, strlen(str));
}

Tokenizer *
tokenizer_new2 (const gchar *str, guint len)
{
    Tokenizer *tok = g_malloc(sizeof(Tokenizer));
    const mecab_node_t *node;

    tok->mecab = mecab_new2("");

    node = mecab_sparse_tonode2(tok->mecab, str, len);
    if (node == NULL){
        return NULL;
    }
    if (node->length == 0){
        node = node->next;
    }
    tok->cur_node = node;

    return tok;
}

void
tokenizer_free (Tokenizer *tok)
{
    mecab_destroy(tok->mecab);
    g_free(tok);
}

const gchar *
tokenizer_next (Tokenizer *tok)
{
    const mecab_node_t *node = tok->cur_node;
    if (node == NULL){
        return NULL;
    }

    while (tok->cur_node != NULL &&
           (tok->cur_node->stat == MECAB_BOS_NODE ||
            tok->cur_node->stat == MECAB_EOS_NODE)){
        tok->cur_node = tok->cur_node->next;
    }

    const gchar *term = g_strndup(node->surface, node->length);
    tok->cur_node = node->next;
    return term;
}
