
static void
hl_cursor_tstr_execute_same_page (hl_cursor *c, hash_leaf *hl)
{
  valid_hash_leaf_assert (hl);
  ASSERT (c->pos < hl->rlen);
  ASSERT (c->vidx + c->vlen == c->vstrlen);

  /**
   * Transition to READING TSTR
   */
  c->state = HLC_READING_TSTR;

  u8 *view = NULL;
  u16 vidx = 0;
  u16 vlen = 0;

  if (c->pos < hl->rlen)
    {
      view = &hl->raw[c->pos];
      vidx = 0;
      vlen = MIN (hl->rlen - c->pos, c->tstrlen);
    }

  c->pos += vlen;

  c->view = view;
  c->vidx = vidx;
  c->vlen = vlen;
}

void
hl_cursor_header_execute (hl_cursor *c, hash_leaf *hl)
{
  ASSERT (c->hidx == 0);
  ASSERT (c->state == HLC_READING_HEAD);

  /**
   * Next chunk to read for the header
   */
  ASSERT (c->hidx <= HLC_PFX_LEN);
  ASSERT (hl->rlen > c->pos);
  p_size next = MIN (HLC_PFX_LEN - c->hidx, hl->rlen - c->pos);

  // Advance position and hidx
  if (next > 0)
    {
      // Read header
      i_memcpy (c->header, &hl->raw[c->pos], next);

      // Move forward in the page
      c->pos += next;

      // Move forward in the header
      c->hidx += next;
    }

  // Half way through the header - need new page
  if (c->hidx != HLC_PFX_LEN)
    {
      c->wants_new_page = true;
      return;
    }

  /**
   * Transition:
   * HLC_READING_HEAD -> HLC_READING_VSTR
   */
  ASSERT (c->state == HLC_READING_HEAD);
  ASSERT (c->hidx == HLC_PFX_LEN);

  {
    /**
     * Interpret Header
     */
    u16 vstrlen = ((u16 *)c->header)[0];

    /**
     * EOF header is all 1's
     */
    if (vstrlen == 0xFF)
      {
        c->state = HLC_EOF;
        return;
      }

    /**
     * Set variables (overrides header)
     */
    c->state = HLC_READING_VSTR;

    c->idx = c->idx; // Unchanged
    c->pos = c->pos; // Unchanged
    c->wants_new_page = c->pos + vstrlen >= hl->rlen;

    c->vstrlen = vstrlen;
    c->tstrlen = tstrlen;
    c->is_present = is_present;
    c->pg0 = pg0;

    c->view = view;
    c->vidx = vidx;
    c->vlen = vlen;
  }
}

hl_cursor
hl_cursor_open (hash_leaf *hl)
{
  valid_hash_leaf_assert (hl);

  hl_cursor ret = {
    .state = HLC_READING_HEAD,
    .idx = 0,
    .pos = (hl->data - hl->raw),
    .wants_new_page = false,
    .hidx = 0,
  };

  /**
   * In general, if we can execute header, we do
   */
  hl_cursor_header_execute (&ret, hl);

  return ret;
}

void
hl_cursor_scroll_same_page (hl_cursor *hc, hash_leaf *hl)
{
  unchecked_hl_cursor_assert (hc);
  valid_hash_leaf_assert (hl);
  ASSERT (!hc->wants_new_page);

  switch (hc->state)
    {
    case HLC_EOF:
    case HLC_READING_HEAD:
      {
        /**
         * If we can execute on header we do
         */
        ASSERT (0);
        break;
      }
    case HLC_READING_VSTR:
      {
        /**
         * MUST transition
         */
        ASSERT (hc->vidx + hc->vlen == hc->vstrlen);
        ASSERT (hc->pos + hc->vlen < hl->rlen);

        hc->pos += hc->vlen;
        hc->view = &hl->raw[hc->pos];
        hc->vidx = 0;

        if (hc->pos + hc->tstrlen <= hl->rlen)
          {
            hc->vlen = hc->tstrlen;
          }
        else
          {
            hc->vlen = hl->rlen - hc->pos;
          }

        // Intersects both cases above
        hc->wants_new_page = hc->pos + hc->tstrlen - hc->vidx >= hl->rlen;
        hc->state = HLC_READING_TSTR;

        break;
      }
    case HLC_READING_TSTR:
      {
        /**
         * MUST transition
         */
        ASSERT (hc->vidx + hc->vlen == hc->tstrlen);
        ASSERT (hc->pos + hc->vlen < hl->rlen);

        hc->idx += 1;
        hc->pos += hc->vlen;
        hc->hidx = 0;
        hc->state = HLC_READING_HEAD;

        hl_cursor_header_execute (hc, hl);
        break;
      }
    }
}

void
hl_cursor_vstr_execute (hl_cursor *hc, hash_leaf *hl)
{
  ASSERT (hc->state == HLC_READING_VSTR);

  /**
   * We are viewing the right half of the string
   * Need to transition
   *
   * Page:
   * [     vidx     vstrend    ][                       ]
   *
   */
  if (hc->vidx + hc->vlen == hc->vstrlen)
    {
      ASSERT (!hc->wants_new_page);
      hc->state = HLC_READING_TSTR;

      hc->pos += hc->vlen;
      hc->vidx = 0;

      if (hc->pos == hl->rlen)
        {
          hc->view = NULL;
        }

      /*
       * Page:
       * [     vidx     vstrend tstr0    ][        tstrend          ]
       *      - want_new_page = true
       */
      if (hc->pos + hc->tstrlen > hl->rlen)
        {
          hc->vlen = hl->rlen - hc->pos; // Might be 0
          hc->wants_new_page = true;
        }

      /*
       * Page:
       * [     vidx     vstrend tstr0    tstrend  ]
       *      - want_new_page = false
       */
      else
        {
          hc->vlen = hc->tstrlen;
          hc->wants_new_page = false;
        }
    }

  /**
   * Otherwise, we still need more string to look at
   *
   * Page:
   * [     vidx       vlen][        vstrend          ]
   */
  else
    {
      ASSERT (hc->wants_new_page);
      ASSERT (hc->pos = (hl->data - hl->raw));
      hc->view = &hl->raw[hc->pos];
    }
}

void
hl_cursor_scroll_new_page (hl_cursor *hc, hash_leaf *hl)
{
  unchecked_hl_cursor_assert (hc);
  valid_hash_leaf_assert (hl);
  ASSERT (hc->wants_new_page);

  hc->pos = hl->data - hl->raw;

  switch (hc->state)
    {
    case HLC_READING_HEAD:
      {
        hc->wants_new_page = false;
        hl_cursor_header_execute (hc, hl);
        break;
      }
    case HLC_READING_VSTR:
      {
        /**
         * Transition
         */
        if (hc->vidx + hc->vlen == hc->vstrlen)
          {
            hc->view = &hl->raw[hc->pos];
            hc->vidx = 0;
            if (hc->pos + hc->tstrlen <= hl->rlen)
              {
                hc->vlen = hc->tstrlen;
              }
            else
              {
                hc->vlen = hl->rlen - hc->pos;
              }

            // Intersects both cases above
            hc->wants_new_page = hc->pos + hc->tstrlen - hc->vidx >= hl->rlen;
            hc->state = HLC_READING_TSTR;
          }

        /**
         * Still looking at vstr
         */
        else
          {
            hc->view = &hl->raw[hc->pos];
            hc->vidx += hc->vlen;
            if (hc->pos + hc->vstrlen <= hl->rlen)
              {
                hc->vlen = hc->vstrlen;
              }
            else
              {
                hc->vlen = hl->rlen - hc->pos;
              }

            // Intersects both cases above
            hc->wants_new_page = hc->pos + hc->vstrlen - hc->vidx >= hl->rlen;
          }

        break;
      }
    case HLC_READING_TSTR:
      {
        /**
         * MUST transition
         */
        ASSERT (hc->vidx + hc->vlen == hc->tstrlen);
        ASSERT (hc->pos + hc->vlen < hl->rlen);

        hc->pos += hc->vlen;
        hc->hidx = 0;
        hc->state = HLC_READING_HEAD;

        hl_cursor_header_execute (hc, hl);
        break;
      }
    case HLC_EOF:
      {
        ASSERT (0);
        return;
      }
    }
}
