/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   simulation.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelsayed <aelsayed@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/04 02:49:28 by aelsayed          #+#    #+#             */
/*   Updated: 2025/06/04 03:23:14 by aelsayed         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../philo.h"

int	is_dead_or_full(t_philo *p)
{
	int	res;

	pthread_mutex_lock(&p->table->meal_check);
	pthread_mutex_lock(&p->table->death_lock);
	res = (p->table->someone_died || p->table->all_full == p->table->nb_philo);
	pthread_mutex_unlock(&p->table->death_lock);
	pthread_mutex_unlock(&p->table->meal_check);
	return (res);
}

void	check_death(t_table *t, long now)
{
	t->i = 0;
	while (t->i < t->nb_philo)
	{
		pthread_mutex_lock(&t->meal_check);
		if (now - t->philos[t->i].last_meal_time >= t->time_to_die)
		{
			pthread_mutex_lock(&t->death_lock);
			t->someone_died = TRUE;
			pthread_mutex_unlock(&t->death_lock);
			pthread_mutex_lock(&t->print_lock);
			printf("%ld %d died\n", get_time() - \
			t->start_time, t->philos[t->i].id);
			pthread_mutex_unlock(&t->print_lock);
			pthread_mutex_unlock(&t->meal_check);
			return ;
		}
		if (t->meals_required != -1 && \
			t->philos[t->i].meals_eaten >= t->meals_required)
			t->all_full++;
		t->i++;
		pthread_mutex_unlock(&t->meal_check);
	}
	if (is_dead_or_full(t->philos))
		return (pthread_mutex_lock(&t->death_lock), \
		t->someone_died = 1, (void)pthread_mutex_unlock(&t->death_lock));
}

void	*monitor(void *table)
{
	t_table	*t;

	t = (t_table *)table;
	while (TRUE)
	{
		pthread_mutex_lock(&t->death_lock);
		if (t->someone_died || t->all_full == t->nb_philo)
		{
			pthread_mutex_unlock(&t->death_lock);
			break ;
		}
		t->all_full = 0;
		pthread_mutex_unlock(&t->death_lock);
		check_death(t, get_time());
		usleep(500);
	}
	return (NULL);
}

void	*routine(void *arg)
{
	t_philo	*p;

	p = (t_philo *)arg;
	if (p->id % 2 == 0)
		usleep(1000);
	while (TRUE)
	{
		if (is_dead_or_full(p))
			return (NULL);
		if (!take_forks(p))
			continue ;
		if (process_eating(p) == FALSE)
			continue ;
		if (process_sleeping(p) == FALSE)
			continue ;
		if (process_thinking(p) == FALSE)
			continue ;
		usleep(1000);
	}
	return (NULL);
}

int	simulation_play(t_table *table)
{
	int	i;

	i = 0;
	table->start_time = get_time();
	while (i < table->nb_philo)
	{
		table->philos[i].last_meal_time = get_time();
		if (pthread_create(&table->philos[i].thread_id, NULL, \
			routine, &table->philos[i]))
			return (table->someone_died = TRUE, \
				destroy_table(table, table->nb_philo, i));
		i++;
	}
	if (pthread_create(&table->monitor, NULL, monitor, table))
		return (table->someone_died = TRUE, \
			destroy_table(table, i, i), FALSE);
	i = 0;
	while (i < table->nb_philo)
		pthread_join(table->philos[i++].thread_id, NULL);
	pthread_join(table->monitor, NULL);
	return (destroy_table(table, table->nb_philo, table->nb_philo), TRUE);
}
