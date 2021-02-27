#include <Arduino.h>
#include "View.h"
#include "State.h"
#include "Buttons.h"
#include "Menutree.h"
#include "chars.h"

void display_state(void);
bool display_menu(int buttons_pressed);
void change_menu_position(int* menu_location, int* menu_selection, int button_pressed);
void assemble_menu_text(char displaied_text[4][21], int menu_location, int menu_selection, int buttons_pressed);
int assemble_func_text(char text[4][21], int menu_location, int menu_selection, int* buttons_pressed);

void controller_update_screen(void)
{
  static bool in_menu = false;
  int nav_buttons = buttons_pushed();

  if(nav_buttons || in_menu)
  {
    in_menu = display_menu(nav_buttons);
  }

  if(!in_menu)
  {
    display_state();
  }
}

void display_state(void){
  static uint8_t previous_state = 0;
  static uint16_t previous_sec = 0;

  uint8_t current_state = state_current_state();
  uint16_t current_sec = 0;

  if(current_state == IDLE_STEP)
  {
    current_sec = (millis() / 1000);
  }
  else
  {
    current_sec = (state_remaining_millis() / 1000);
  }

  if ((current_state != previous_state) || (current_sec != previous_sec))
  {
    view_display_state();
    previous_state = current_state;
    previous_sec = current_sec;
  }
}

bool display_menu(int buttons_pressed)
{
  static bool in_menu = false;
  static int menu_location = -1;
  static int menu_selection = 0;
  static long interaction_at = 0;
  static long menu_timeout = 5000;
  static bool in_function = false;
  static long callback_time = 0;
  static long callback_set = 0;
  static char menu_text[4][21];
  bool callback_flag = false;

  if(buttons_pressed)
  {
    interaction_at = millis();
  }
  else if(!in_function && (millis() - interaction_at) > menu_timeout)
  {
    menu_location = -1;
    in_menu = false;
  }

  if(in_function && callback_time && ((millis() - callback_set) > callback_time))
  {
    callback_flag = true;
  }
  else
  {
    callback_flag = false;
  }

  if(callback_flag || buttons_pressed)
  {
    for(int i = 0; i < 4; i++)
    {
      clear_char_array(menu_text[i], 21);
    }

    if(!in_function)
    {
      change_menu_position(&menu_location, &menu_selection, buttons_pressed);
    }

    if((menu_location >= MENU_ROOT) && (menu_is_function(menu_location)))
    {
      callback_time = assemble_func_text(menu_text, menu_location, menu_selection, &buttons_pressed);
      callback_set = millis();
      in_function = true;

      if(buttons_pressed & BUTTON_LEFT)
      {
        change_menu_position(&menu_location, &menu_selection, buttons_pressed);
        in_function = false;
      }
    }

    if((menu_location >= MENU_ROOT) && (!in_function))
    {
      for(int i = 0; i < 4; i++)
      {
        clear_char_array(menu_text[i], 21);
      }
      assemble_menu_text(menu_text, menu_location, menu_selection, buttons_pressed);
      in_menu = true;
    }
    else
    {
      in_menu = false;
    }

    if(in_menu || in_function)
    {
      view_clear();
      for(int i = 0; i < 4; i++)
      {
        view_println(menu_text[i]);
      }
    }
  }

  //(menu_location >= MENU_ROOT) ? (return_value = true) : (return_value = false);
  return (in_menu || in_function);//return_value;
}

void change_menu_position(int* menu_location, int* menu_selection, int buttons_pressed)
{
  if((*menu_location < MENU_ROOT) && (buttons_pressed & BUTTON_RIGHT))
  {
    *menu_location = MENU_ROOT;
    *menu_selection = menu_get_child(*menu_location);
  }
  else if((*menu_location >= MENU_ROOT) && (buttons_pressed & BUTTON_RIGHT))
  {
    int temp = *menu_selection;
    if(temp > MENU_ROOT)
      *menu_location = temp;

    *menu_selection = menu_get_child(*menu_location);
  }
  else if((*menu_location >= MENU_ROOT) && (buttons_pressed & BUTTON_LEFT))
  {
    *menu_selection = *menu_location;
    *menu_location = menu_get_parent(*menu_location);
  }
  else if((*menu_location >= MENU_ROOT) && (buttons_pressed & BUTTON_DOWN))
  {
    int temp = menu_get_next_sibling(*menu_selection);
    if(temp > MENU_ROOT)
      *menu_selection = temp;
  }
  else if((*menu_location >= MENU_ROOT) && (buttons_pressed & BUTTON_UP))
  {
    int temp = menu_get_prev_sibling(*menu_selection);
    if(temp > MENU_ROOT)
      *menu_selection = temp;
  }
}

void assemble_menu_text(char displaied_text[4][21], int menu_location, int menu_selection, int buttons_pressed)
{
  int displayed_menu = menu_get_child(menu_location);
  bool selection_in_range = false;
  bool at_end = false;

  //where to start display to make selection visible
  while(selection_in_range == false)
  {
    int test_point = displayed_menu;
    for(int i = 1; i < 4; i++)
    {
      if(0 == (menu_selection - test_point))
      {
        selection_in_range = true;
        if(i == (4 - 1))
          at_end = true;
      }
      test_point = menu_get_next_sibling(test_point);
    }

    if(!selection_in_range)
    {
      displayed_menu = menu_get_next_sibling(displayed_menu);
    }
  }

  if(at_end && (menu_get_child(menu_location) != menu_get_next_sibling(menu_selection)))
  {
    displayed_menu = menu_get_next_sibling(displayed_menu);
  }

  char_concatenate(displaied_text[0], "<", menu_get_name(menu_location), 21);
  char_concatenate(displaied_text[0], displaied_text[0], ">", 21);

  for(int i = 1; i < 4; i++)
  {
    if(displayed_menu == menu_selection)// && displayed_menu != 0)
    {
      char_concatenate(displaied_text[i], ">", menu_get_name(displayed_menu), 21);
    }
    else if(displayed_menu)
    {
      char_concatenate(displaied_text[i], " ", menu_get_name(displayed_menu), 21);
    }

    displayed_menu = menu_get_next_sibling(displayed_menu);
  }
}

int assemble_func_text(char text[4][21], int menu_location, int menu_selection, int* buttons_pressed)
{
      static int return_value = 0;
      char_concatenate(text[0], "<", menu_get_name(menu_location), 21);
      char_concatenate(text[0], text[0], ">", 21);
      copy_char_array(text[2], "function = nullptr", 21);
      menu_function(menu_location, text, buttons_pressed);

      return return_value;
}
