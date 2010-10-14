
#include "inv-index.h"

Tokenizer *
tokenizer_new (const gchar *str)
{
    if (!str) return NULL;
    return tokenizer_new2(str, strlen(str));
}

Tokenizer *
tokenizer_new2 (const gchar *str, guint len)
{
    if (!str) return NULL;
    Tokenizer *tok = g_malloc(sizeof(Tokenizer));
    const mecab_node_t *node;
    tok->mecab = mecab_new2("");

//    node = mecab_sparse_tonode2(tok->mecab, str, len);
//    if (node == NULL){
//        return NULL;
//    }
//    if (node->length == 0){
//        node = node->next;
//    }
//    tok->cur_node = node;
//
//    return tok;

    tokenizer_renew2(tok, str, len);
}

Tokenizer *
tokenizer_renew  (Tokenizer *tok, const gchar *str)
{
    return tokenizer_renew2(tok, str, strlen(str));
}

Tokenizer *
tokenizer_renew2 (Tokenizer *tok, const gchar *str, guint len)
{
    if (!tok) {
        return tokenizer_new2(str, len);
    }
    const mecab_node_t *node;
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

    while (node != NULL &&
           (node->stat == MECAB_BOS_NODE ||
            node->stat == MECAB_EOS_NODE)){
        tok->cur_node = node->next;
    }

    const gchar *term;
    if (node->length > 0){
        term = g_strndup(node->surface, node->length);
    } else {
        term = NULL;
    }
    tok->cur_node = node->next;

    return term;
}
